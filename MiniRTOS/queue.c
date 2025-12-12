#include <stdlib.h>
#include <string.h>

#include "MiniRTOS.h"
#include "task.h"
#include "queue.h"

#define queueUNLOCKED ((int8_t)-1)
#define queueLOCKED_UNMODIFIED ((int8_t)0)

#define queueYIELD() portYIELD()

#define prvLockQueue(pxQueue)                            \
    taskENTER_CRITICAL();                                \
    {                                                    \
        if ((pxQueue)->cRxLock == queueUNLOCKED)         \
        {                                                \
            (pxQueue)->cRxLock = queueLOCKED_UNMODIFIED; \
        }                                                \
        if ((pxQueue)->cTxLock == queueUNLOCKED)         \
        {                                                \
            (pxQueue)->cTxLock = queueLOCKED_UNMODIFIED; \
        }                                                \
    }                                                    \
    taskEXIT_CRITICAL()

typedef struct QueueDefinition
{
    int8_t *pcHead;
    int8_t *pcTail;
    int8_t *pcWriteTo;
    int8_t *pcReadFrom;

    List_t xTasksWaitingToSend;
    List_t xTasksWaitingToReceive;

    volatile UBaseType_t uxMessagesWaiting;
    UBaseType_t uxLength;
    UBaseType_t uxItemSize;

    volatile int8_t cRxLock;
    volatile int8_t cTxLock;
} xQUEUE;
typedef xQUEUE Queue_t;

static void prvUnlockQueue(Queue_t *const pxQueue)
{
    /* THIS FUNCTION MUST BE CALLED WITH THE SCHEDULER SUSPENDED. */

    /* The lock counts contains the number of extra data items placed or
    removed from the queue while the queue was locked.  When a queue is
    locked items can be added or removed, but the event lists cannot be
    updated. */
    taskENTER_CRITICAL();
    {
        int8_t cTxLock = pxQueue->cTxLock;

        /* See if data was added to the queue while it was locked. */
        while (cTxLock > queueLOCKED_UNMODIFIED)
        {
            /* Data was posted while the queue was locked.  Are any tasks
            blocked waiting for data to become available? */

            /* Tasks that are removed from the event list will get added to
            the pending ready list as the scheduler is still suspended. */
            if (listLIST_IS_EMPTY(&(pxQueue->xTasksWaitingToReceive)) == pdFALSE)
            {
                if (xTaskRemoveFromEventList(&(pxQueue->xTasksWaitingToReceive)) != pdFALSE)
                {
                    /* The task waiting has a higher priority so record that
                    a context switch is required. */
                    vTaskMissedYield();
                }
            }
            else
            {
                break;
            }

            --cTxLock;
        }

        pxQueue->cTxLock = queueUNLOCKED;
    }
    taskEXIT_CRITICAL();

    /* Do the same for the Rx lock. */
    taskENTER_CRITICAL();
    {
        int8_t cRxLock = pxQueue->cRxLock;

        while (cRxLock > queueLOCKED_UNMODIFIED)
        {
            if (listLIST_IS_EMPTY(&(pxQueue->xTasksWaitingToSend)) == pdFALSE)
            {
                if (xTaskRemoveFromEventList(&(pxQueue->xTasksWaitingToSend)) != pdFALSE)
                {
                    vTaskMissedYield();
                }

                --cRxLock;
            }
            else
            {
                break;
            }
        }

        pxQueue->cRxLock = queueUNLOCKED;
    }
    taskEXIT_CRITICAL();
}

/*队列创建相关*/
static BaseType_t xQueueReset(QueueHandle_t xQueue)
{
    Queue_t *const pxQueue = (Queue_t *)xQueue;

    configASSERT(pxQueue);

    taskENTER_CRITICAL();
    {
        pxQueue->pcTail = pxQueue->pcHead + (pxQueue->uxLength * pxQueue->uxItemSize);
        pxQueue->uxMessagesWaiting = (UBaseType_t)0U;
        pxQueue->pcWriteTo = pxQueue->pcHead;
        pxQueue->pcReadFrom = pxQueue->pcHead + ((pxQueue->uxLength - (UBaseType_t)1U) * pxQueue->uxItemSize);
        pxQueue->cRxLock = queueUNLOCKED;
        pxQueue->cTxLock = queueUNLOCKED;

        vListInitialise(&(pxQueue->xTasksWaitingToSend));
        vListInitialise(&(pxQueue->xTasksWaitingToReceive));
    }
    taskEXIT_CRITICAL();

    return pdPASS;
}

static void prvInitialiseNewQueue(const UBaseType_t uxQueueLength, const UBaseType_t uxItemSize, uint8_t *pucQueueStorage, Queue_t *pxNewQueue)
{
    pxNewQueue->pcHead = (int8_t *)pucQueueStorage;

    pxNewQueue->uxLength = uxQueueLength;
    pxNewQueue->uxItemSize = uxItemSize;
    (void)xQueueReset(pxNewQueue);
}

/**
 * @brief 创建队列
 */
QueueHandle_t xQueueCreate(const UBaseType_t uxQueueLength, const UBaseType_t uxItemSize)
{
    Queue_t *pxNewQueue;
    size_t xQueueSizeInBytes;
    uint8_t *pucQueueStorage;

    configASSERT(uxQueueLength > (UBaseType_t)0);
    configASSERT(uxItemSize);

    xQueueSizeInBytes = (size_t)(uxQueueLength * uxItemSize);

    pxNewQueue = (Queue_t *)pvPortMalloc(sizeof(Queue_t) + xQueueSizeInBytes);

    if (pxNewQueue != NULL)
    {
        pucQueueStorage = ((uint8_t *)pxNewQueue) + sizeof(Queue_t);

        prvInitialiseNewQueue(uxQueueLength, uxItemSize, pucQueueStorage, pxNewQueue);
    }

    return pxNewQueue;
}
/*-----------------------------------------------------------*/

/*队列发送相关*/
static void prvCopyDataToQueue(Queue_t *const pxQueue, const void *pvItemToQueue)
{
    UBaseType_t uxMessagesWaiting;

    uxMessagesWaiting = pxQueue->uxMessagesWaiting;

    (void)memcpy((void *)pxQueue->pcWriteTo, pvItemToQueue, (size_t)pxQueue->uxItemSize); /*lint !e961 !e418 MISRA exception as the casts are only redundant for some ports, plus previous logic ensures a null pointer can only be passed to memcpy() if the copy size is 0. */
    pxQueue->pcWriteTo += pxQueue->uxItemSize;
    if (pxQueue->pcWriteTo >= pxQueue->pcTail)
    {
        pxQueue->pcWriteTo = pxQueue->pcHead;
    }

    pxQueue->uxMessagesWaiting = uxMessagesWaiting + 1;
}

static BaseType_t prvIsQueueFull(const Queue_t *pxQueue)
{
    BaseType_t xReturn;

    taskENTER_CRITICAL();
    {
        if (pxQueue->uxMessagesWaiting == pxQueue->uxLength)
        {
            xReturn = pdTRUE;
        }
        else
        {
            xReturn = pdFALSE;
        }
    }
    taskEXIT_CRITICAL();

    return xReturn;
}

/**
 * @brief 向队列发送数据
 */
BaseType_t xQueueSend(QueueHandle_t xQueue, const void *const pvItemToQueue, TickType_t xTicksToWait)
{
    BaseType_t xEntryTimeSet = pdFALSE;
    TimeOut_t xTimeOut;
    Queue_t *const pxQueue = (Queue_t *)xQueue;

    configASSERT(pxQueue);
    configASSERT(pvItemToQueue);

    for (;;)
    {
        taskENTER_CRITICAL();
        {
            if ((pxQueue->uxMessagesWaiting < pxQueue->uxLength))
            {
                prvCopyDataToQueue(pxQueue, pvItemToQueue);

                if (listLIST_IS_EMPTY(&(pxQueue->xTasksWaitingToReceive)) == pdFALSE)
                {
                    if (xTaskRemoveFromEventList(&(pxQueue->xTasksWaitingToReceive)) != pdFALSE)
                    {
                        queueYIELD();
                    }
                }

                taskEXIT_CRITICAL();
                return pdPASS;
            }
            else
            {
                if (xTicksToWait == (TickType_t)0)
                {
                    taskEXIT_CRITICAL();

                    return errQUEUE_FULL;
                }
                else if (xEntryTimeSet == pdFALSE) /*在循环中仅运行一次*/
                {
                    vTaskSetTimeOutState(&xTimeOut);
                    xEntryTimeSet = pdTRUE;
                }
            }
        }
        taskEXIT_CRITICAL();

        vTaskSuspendAll();
        prvLockQueue(pxQueue);

        if (xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) == pdFALSE)
        {
            if (prvIsQueueFull(pxQueue) != pdFALSE)
            {
                vTaskPlaceOnEventList(&(pxQueue->xTasksWaitingToSend), xTicksToWait);

                prvUnlockQueue(pxQueue);

                if (xTaskResumeAll() == pdFALSE)
                {
                    queueYIELD();
                }
            }
            else
            {
                prvUnlockQueue(pxQueue); /*再试一次*/
                (void)xTaskResumeAll();
            }
        }
        else
        {
            prvUnlockQueue(pxQueue);
            (void)xTaskResumeAll();

            return errQUEUE_FULL;
        }
    }
}
/*-----------------------------------------------------------*/

/*队列接收相关*/
static void prvCopyDataFromQueue(Queue_t *const pxQueue, void *const pvBuffer)
{
    pxQueue->pcReadFrom += pxQueue->uxItemSize;
    if (pxQueue->pcReadFrom >= pxQueue->pcTail) /*lint !e946 MISRA exception justified as use of the relational operator is the cleanest solutions. */
    {
        pxQueue->pcReadFrom = pxQueue->pcHead;
    }

    (void)memcpy((void *)pvBuffer, (void *)pxQueue->pcReadFrom, (size_t)pxQueue->uxItemSize); /*lint !e961 !e418 MISRA exception as the casts are only redundant for some ports.  Also previous logic ensures a null pointer can only be passed to memcpy() when the count is 0. */
}

static BaseType_t prvIsQueueEmpty(const Queue_t *pxQueue)
{
    BaseType_t xReturn;

    taskENTER_CRITICAL();
    {
        if (pxQueue->uxMessagesWaiting == (UBaseType_t)0)
        {
            xReturn = pdTRUE;
        }
        else
        {
            xReturn = pdFALSE;
        }
    }
    taskEXIT_CRITICAL();

    return xReturn;
}

/**
 * @brief 从队列接收数据
 */
BaseType_t xQueueReceive(QueueHandle_t xQueue, void *const pvBuffer, TickType_t xTicksToWait)
{
    BaseType_t xEntryTimeSet = pdFALSE;
    TimeOut_t xTimeOut;
    Queue_t *const pxQueue = (Queue_t *)xQueue;

    configASSERT(pxQueue);
    configASSERT(pvBuffer);

    for (;;)
    {
        taskENTER_CRITICAL();
        {
            const UBaseType_t uxMessagesWaiting = pxQueue->uxMessagesWaiting;

            if (uxMessagesWaiting > (UBaseType_t)0)
            {
                prvCopyDataFromQueue(pxQueue, pvBuffer);

                pxQueue->uxMessagesWaiting = uxMessagesWaiting - 1;

                if (listLIST_IS_EMPTY(&(pxQueue->xTasksWaitingToSend)) == pdFALSE)
                {
                    if (xTaskRemoveFromEventList(&(pxQueue->xTasksWaitingToSend)) != pdFALSE)
                    {
                        queueYIELD();
                    }
                }

                taskEXIT_CRITICAL();
                return pdPASS;
            }
            else
            {
                if (xTicksToWait == (TickType_t)0)
                {
                    taskEXIT_CRITICAL();
                    return errQUEUE_EMPTY;
                }
                else if (xEntryTimeSet == pdFALSE)
                {
                    vTaskSetTimeOutState(&xTimeOut);
                    xEntryTimeSet = pdTRUE;
                }
            }
        }
        taskEXIT_CRITICAL();

        vTaskSuspendAll();
        prvLockQueue(pxQueue);

        if (xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) == pdFALSE)
        {
            if (prvIsQueueEmpty(pxQueue) != pdFALSE)
            {
                vTaskPlaceOnEventList(&(pxQueue->xTasksWaitingToReceive), xTicksToWait);
                prvUnlockQueue(pxQueue);
                if (xTaskResumeAll() == pdFALSE)
                {
                    portYIELD();
                }
            }
            else
            {
                /* Try again. */
                prvUnlockQueue(pxQueue);
                (void)xTaskResumeAll();
            }
        }
        else
        {
            prvUnlockQueue(pxQueue);
            (void)xTaskResumeAll();

            if (prvIsQueueEmpty(pxQueue) != pdFALSE)
            {
                return errQUEUE_EMPTY;
            }
        }
    }
}
/*-----------------------------------------------------------*/
