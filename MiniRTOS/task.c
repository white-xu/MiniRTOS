#include <stdlib.h>
#include <string.h>

#include "MiniRTOS.h"
#include "task.h"

#define taskRECORD_READY_PRIORITY( uxPriority )	portRECORD_READY_PRIORITY( uxPriority, uxTopReadyPriority )
#define taskSELECT_HIGHEST_PRIORITY_TASK()                                              \
    {                                                                                   \
        UBaseType_t uxTopPriority;                                                      \
                                                                                        \
        /* Find the highest priority list that contains ready tasks. */                 \
        portGET_HIGHEST_PRIORITY(uxTopPriority, uxTopReadyPriority);                    \
        configASSERT(listCURRENT_LIST_LENGTH(&(pxReadyTasksLists[uxTopPriority])) > 0); \
        listGET_OWNER_OF_NEXT_ENTRY(pxCurrentTCB, &(pxReadyTasksLists[uxTopPriority])); \
    }

#define prvAddTaskToReadyList( pxTCB )																    \
	taskRECORD_READY_PRIORITY( ( pxTCB )->uxPriority );												    \
	vListInsertEnd( &( pxReadyTasksLists[ ( pxTCB )->uxPriority ] ), &( ( pxTCB )->xStateListItem ) );  \

/**
 * @brief 任务控制块
 */
typedef struct tskTaskControlBlock
{
    volatile StackType_t *pxTopOfStack;
    ListItem_t xStateListItem;
    UBaseType_t uxPriority;
    StackType_t *pxStack;
    char pcTaskName[16];
} tskTCB;
typedef tskTCB TCB_t;

/**
 * @brief 指向当前正在运行的任务控制块
 */
TCB_t *volatile pxCurrentTCB = NULL;

/*状态链表*/
static List_t pxReadyTasksLists[32];
static List_t xDelayedTaskList1;
static List_t xDelayedTaskList2;
static List_t *volatile pxDelayedTaskList;
static List_t *volatile pxOverflowDelayedTaskList;

static volatile UBaseType_t uxCurrentNumberOfTasks = (UBaseType_t)0U;       /*当前任务数量*/
static volatile UBaseType_t uxTopReadyPriority = tskIDLE_PRIORITY;          /*当前优先级最高任务的优先级*/
static volatile BaseType_t xSchedulerRunning = pdFALSE;                     /*指示调度器状态*/
static volatile TickType_t xTickCount = (TickType_t)0U;                     /*系统滴答定时值*/
static TaskHandle_t xIdleTaskHandle = NULL;                                 /*空闲任务句柄*/

static void prvInitialiseNewTask(TaskFunction_t pxTaskCode,
                                 const char *const pcName,
                                 const uint32_t ulStackDepth,
                                 void *const pvParameters,
                                 UBaseType_t uxPriority,
                                 TaskHandle_t *const pxCreatedTask,
                                 TCB_t *pxNewTCB)
{
    StackType_t *pxTopOfStack;
    UBaseType_t x;

    /*获取栈顶指针、对齐*/
    pxTopOfStack = pxNewTCB->pxStack + (ulStackDepth - (uint32_t)1);
    pxTopOfStack = (StackType_t *)(((uint32_t)pxTopOfStack) & (~((uint32_t)portBYTE_ALIGNMENT_MASK)));
    configASSERT((((uint32_t)pxTopOfStack & (uint32_t)portBYTE_ALIGNMENT_MASK) == 0UL));

    /*存储任务名*/
    for (x = (UBaseType_t)0; x < (UBaseType_t)16; x++)
    {
        pxNewTCB->pcTaskName[x] = pcName[x];

        if (pcName[x] == 0x00)
        {
            break;
        }
    }
    pxNewTCB->pcTaskName[15] = '\0';

    /*存储优先级*/
    pxNewTCB->uxPriority = uxPriority;

    /*配置状态链表项*/
    vListInitialiseItem(&(pxNewTCB->xStateListItem));
    listSET_LIST_ITEM_OWNER(&(pxNewTCB->xStateListItem), pxNewTCB);

    /*初始化任务栈*/
    pxNewTCB->pxTopOfStack = pxPortInitialiseStack(pxTopOfStack, pxTaskCode, pvParameters);

    if ((void *)pxCreatedTask != NULL)
    {
        *pxCreatedTask = (TaskHandle_t)pxNewTCB;
    }
}

static void prvInitialiseTaskLists(void)
{
    UBaseType_t uxPriority;

    for (uxPriority = (UBaseType_t)0U; uxPriority < (UBaseType_t)32; uxPriority++)
    {
        vListInitialise(&(pxReadyTasksLists[uxPriority]));
    }

    vListInitialise(&xDelayedTaskList1);
    vListInitialise(&xDelayedTaskList2);
    pxDelayedTaskList = &xDelayedTaskList1;
    pxOverflowDelayedTaskList = &xDelayedTaskList2;
}

static void prvAddNewTaskToReadyList(TCB_t *pxNewTCB)
{
    taskENTER_CRITICAL();
    {
        uxCurrentNumberOfTasks++;
        if (pxCurrentTCB == NULL)
        {
            pxCurrentTCB = pxNewTCB;
            if (uxCurrentNumberOfTasks == (UBaseType_t)1)
            {
                prvInitialiseTaskLists();
            }
        }
        else
        {
            if (xSchedulerRunning == pdFALSE)
            {
                if (pxCurrentTCB->uxPriority <= pxNewTCB->uxPriority)
                {
                    pxCurrentTCB = pxNewTCB;
                }
            }
        }

        prvAddTaskToReadyList(pxNewTCB);
    }
    taskEXIT_CRITICAL();

    if (xSchedulerRunning != pdFALSE)
    {
        if (pxCurrentTCB->uxPriority < pxNewTCB->uxPriority)
        {
            taskYIELD();
        }
    }
}

/**
 * @brief 创建任务
 */
BaseType_t xTaskCreate(TaskFunction_t pxTaskCode,
                       const char *const pcName,
                       const uint16_t usStackDepth,
                       void *const pvParameters,
                       UBaseType_t uxPriority,
                       TaskHandle_t *const pxCreatedTask)
{
    BaseType_t xReturn;
    TCB_t *pxNewTCB;
    StackType_t *pxStack;

    /*分配任务栈*/
    pxStack = (StackType_t *)pvPortMalloc((((size_t)usStackDepth) * sizeof(StackType_t)));
    if (pxStack != NULL)
    {
        pxNewTCB = (TCB_t *)pvPortMalloc(sizeof(TCB_t));
        if (pxNewTCB != NULL)
        {
            pxNewTCB->pxStack = pxStack;
        }
        else
        {
            vPortFree(pxStack);
        }
    }
    else
    {
        pxNewTCB = NULL;
    }

    /*初始化创建的任务、将任务添加到就绪链表*/
    if (pxNewTCB != NULL)
    {
        prvInitialiseNewTask(pxTaskCode, pcName, (uint32_t)usStackDepth, pvParameters, uxPriority, pxCreatedTask, pxNewTCB);
        prvAddNewTaskToReadyList(pxNewTCB);
        xReturn = pdPASS;
    }
    else
    {
        xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
    }

    return xReturn;
}

static void prvIdleTask(void *pvParameters)
{
    /*Stop warnings.*/
    (void)pvParameters;

    for (;;)
    {
        if (listCURRENT_LIST_LENGTH(&(pxReadyTasksLists[tskIDLE_PRIORITY])) > (UBaseType_t)1)
        {
            taskYIELD();
        }
    }
}

/**
 * @brief 启动调度器
 */
void vTaskStartScheduler(void)
{
    BaseType_t xReturn;

    /*创建空闲任务*/
    xReturn = xTaskCreate(prvIdleTask,
                          "IDLE", configMINIMAL_STACK_SIZE,
                          (void *)NULL,
                          (tskIDLE_PRIORITY),
                          &xIdleTaskHandle);

    if (xReturn == pdPASS)
    {
        /*在这里，中断被关闭，以确保在调用xPortStartScheduler（）之前或期间不会出现标记。
        所创建任务的堆栈包含一个状态字，该状态字将中断打开，因此当第一个任务开始运行时，中断将自动重新启用。*/
        portDISABLE_INTERRUPTS();

        // xNextTaskUnblockTime = portMAX_DELAY;
        xSchedulerRunning = pdTRUE;
        xTickCount = (TickType_t)0U;

        if (xPortStartScheduler() != pdFALSE)
        {
            /* Should not reach here as if the scheduler is running the
            function will not return. */
        }
        else
        {
            /* Should only reach here if a task calls xTaskEndScheduler(). */
        }
    }
    else
    {
        configASSERT(xReturn != errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY);
    }
}

/**
 * @brief 递增滴答值、同等优先级任务共享处理时间(阉割版)
 */
BaseType_t xTaskIncrementTick(void)
{
    BaseType_t xSwitchRequired = pdFALSE;

    const TickType_t xConstTickCount = xTickCount + 1;
	xTickCount = xConstTickCount;                           /*避免编译器优化问题*/

    if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[ pxCurrentTCB->uxPriority ] ) ) > ( UBaseType_t ) 1 )
	{
		xSwitchRequired = pdTRUE;
	}

    return xSwitchRequired;
}

void vTaskSwitchContext(void)
{
	/* Select a new task to run using either the generic C or port
		optimised asm code. */
	taskSELECT_HIGHEST_PRIORITY_TASK();
}
