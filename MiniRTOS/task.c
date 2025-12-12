#include <stdlib.h>
#include <string.h>

#include "MiniRTOS.h"
#include "task.h"

#define taskRECORD_READY_PRIORITY(uxPriority) portRECORD_READY_PRIORITY(uxPriority, uxTopReadyPriority)
#define taskSELECT_HIGHEST_PRIORITY_TASK()                                              \
    {                                                                                   \
        UBaseType_t uxTopPriority;                                                      \
                                                                                        \
        /* Find the highest priority list that contains ready tasks. */                 \
        portGET_HIGHEST_PRIORITY(uxTopPriority, uxTopReadyPriority);                    \
        configASSERT(listCURRENT_LIST_LENGTH(&(pxReadyTasksLists[uxTopPriority])) > 0); \
        listGET_OWNER_OF_NEXT_ENTRY(pxCurrentTCB, &(pxReadyTasksLists[uxTopPriority])); \
    }
#define taskRESET_READY_PRIORITY(uxPriority)                                               \
    {                                                                                      \
        if (listCURRENT_LIST_LENGTH(&(pxReadyTasksLists[(uxPriority)])) == (UBaseType_t)0) \
        {                                                                                  \
            portRESET_READY_PRIORITY((uxPriority), (uxTopReadyPriority));                  \
        }                                                                                  \
    }

#define prvAddTaskToReadyList(pxTCB)                \
    taskRECORD_READY_PRIORITY((pxTCB)->uxPriority); \
    vListInsertEnd(&(pxReadyTasksLists[(pxTCB)->uxPriority]), &((pxTCB)->xStateListItem));

#define taskSWITCH_DELAYED_LISTS()                                                \
    {                                                                             \
        List_t *pxTemp;                                                           \
                                                                                  \
        /* The delayed tasks list should be empty when the lists are switched. */ \
        configASSERT((listLIST_IS_EMPTY(pxDelayedTaskList)));                     \
                                                                                  \
        pxTemp = pxDelayedTaskList;                                               \
        pxDelayedTaskList = pxOverflowDelayedTaskList;                            \
        pxOverflowDelayedTaskList = pxTemp;                                       \
        xNumOfOverflows++;                                                        \
        prvResetNextTaskUnblockTime();                                            \
    }

/**
 * @brief 任务控制块
 */
typedef struct tskTaskControlBlock
{
    volatile StackType_t *pxTopOfStack;
    ListItem_t xStateListItem;
    ListItem_t xEventListItem;
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
static List_t pxReadyTasksLists[32];               /*就绪链表*/
static List_t xDelayedTaskList1;                   /*延时链表1*/
static List_t xDelayedTaskList2;                   /*延时链表2*/
static List_t *volatile pxDelayedTaskList;         /*指向当前正在使用的延时链表*/
static List_t *volatile pxOverflowDelayedTaskList; /*指向溢出的延时链表*/
static List_t xPendingReadyList;                   /*调度器挂起期间已经就绪的任务列表。这些任务将在调度器恢复时被移动到就绪列表中*/

static volatile UBaseType_t uxCurrentNumberOfTasks = (UBaseType_t)0U; /*当前任务数量*/
static volatile UBaseType_t uxTopReadyPriority = tskIDLE_PRIORITY;    /*优先级位图*/
static volatile BaseType_t xSchedulerRunning = pdFALSE;               /*指示调度器状态*/
static volatile TickType_t xTickCount = (TickType_t)0U;               /*系统滴答值*/
static volatile BaseType_t xNumOfOverflows = (BaseType_t)0;           /*xTickCount溢出次数*/
static volatile TickType_t xNextTaskUnblockTime = (TickType_t)0U;     /*下一次解除阻塞的时间*/
static volatile BaseType_t xYieldPending = pdFALSE;                   /*请求调度器进行一次任务切换*/
static volatile UBaseType_t uxPendedTicks = (UBaseType_t)0U;          /*调度器挂起期间未处理的滴答中断计数*/
static TaskHandle_t xIdleTaskHandle = NULL;                           /*空闲任务句柄*/

/*当调度器被挂起时,上下文切换会延迟等待。中断不得操作任务控制块(TCB)的 xStateListItem,也不得操作xStateListItem所关联的任何链表。
如果中断需要在调度器挂起期间解除某个任务的阻塞,它会将任务的事件列表项移动到xPendingReadyList(待处理就绪列表)中,以便内核在调度器恢
复时,将任务从待处理就绪列表移入真正的就绪列表。待处理就绪列表本身只能在临界区(critical section)中访问*/
static volatile UBaseType_t uxSchedulerSuspended = (UBaseType_t)pdFALSE;

/**/
static void prvResetNextTaskUnblockTime(void);
/*-----------------------------------------------------------*/

/*任务创建*/
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

    /*配置事件链表项*/
    vListInitialiseItem(&(pxNewTCB->xEventListItem));
    listSET_LIST_ITEM_VALUE(&(pxNewTCB->xEventListItem), (TickType_t)32 - (TickType_t)uxPriority); /*保证高优先级的排列的前面(链表项是升序排列的)*/
    listSET_LIST_ITEM_OWNER(&(pxNewTCB->xEventListItem), pxNewTCB);

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
/*-----------------------------------------------------------*/

/*调度器相关*/
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

        xNextTaskUnblockTime = portMAX_DELAY;
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
 * @brief 暂停调度器但不关闭中断。在调度器被暂停期间,不会发生上下文切换
 * @note  在调用vTaskSuspendAll()之后,调用该程序的任务将继续执行,期间不会面临被换出的风险,直到调用xTaskResumeAll()为止
 *        那些可能导致上下文切换的API函数(例如vTaskDelay()等)在调度器处于暂停状态时不应该被调用
 */
void vTaskSuspendAll(void)
{
    /*此处不需要临界区保护,因为该变量为BaseType_t类型*/
    ++uxSchedulerSuspended;
}

/**
 * @brief 恢复调度器
 * @note  xTaskResumeAll()仅会恢复调度器,它不会解除之前因调用vTaskSuspendAll而被暂停的任务的暂停状态
 */
BaseType_t xTaskResumeAll(void)
{
    TCB_t *pxTCB = NULL;
    BaseType_t xAlreadyYielded = pdFALSE;

    configASSERT(uxSchedulerSuspended);

    taskENTER_CRITICAL();
    {
        --uxSchedulerSuspended;

        if (uxSchedulerSuspended == (UBaseType_t)pdFALSE)
        {
            if (uxCurrentNumberOfTasks > (UBaseType_t)0U)
            {
                while (listLIST_IS_EMPTY(&xPendingReadyList) == pdFALSE)
                {
                    pxTCB = (TCB_t *)listGET_OWNER_OF_HEAD_ENTRY((&xPendingReadyList));
                    (void)uxListRemove(&(pxTCB->xEventListItem));
                    (void)uxListRemove(&(pxTCB->xStateListItem));
                    prvAddTaskToReadyList(pxTCB);

                    if (pxTCB->uxPriority >= pxCurrentTCB->uxPriority)
                    {
                        xYieldPending = pdTRUE;
                    }
                }

                if (pxTCB != NULL)
                {
                    prvResetNextTaskUnblockTime(); /*更新xNextTaskUnblockTime*/
                }

                {
                    UBaseType_t uxPendedCounts = uxPendedTicks;

                    if (uxPendedCounts > (UBaseType_t)0U)
                    {
                        do
                        {
                            if (xTaskIncrementTick() != pdFALSE)
                            {
                                xYieldPending = pdTRUE;
                            }
                            --uxPendedCounts;
                        } while (uxPendedCounts > (UBaseType_t)0U);

                        uxPendedTicks = 0;
                    }
                }

                if (xYieldPending != pdFALSE)
                {
                    xAlreadyYielded = pdTRUE;

                    taskYIELD();
                }
            }
        }
    }
    taskEXIT_CRITICAL();

    return xAlreadyYielded;
}
/*-----------------------------------------------------------*/

/*任务延时*/
static void prvResetNextTaskUnblockTime(void)
{
    TCB_t *pxTCB;

    if (listLIST_IS_EMPTY(pxDelayedTaskList) != pdFALSE)
    {
        xNextTaskUnblockTime = portMAX_DELAY;
    }
    else
    {
        (pxTCB) = (TCB_t *)listGET_OWNER_OF_HEAD_ENTRY(pxDelayedTaskList);
        xNextTaskUnblockTime = listGET_LIST_ITEM_VALUE(&((pxTCB)->xStateListItem));
    }
}

static void prvAddCurrentTaskToDelayedList(TickType_t xTicksToWait)
{
    TickType_t xTimeToWake;
    const TickType_t xConstTickCount = xTickCount;

    /*将当前任务从就绪链表中移除*/
    if (uxListRemove(&(pxCurrentTCB->xStateListItem)) == (UBaseType_t)0)
    {
        portRESET_READY_PRIORITY(pxCurrentTCB->uxPriority, uxTopReadyPriority); /*该优先级的链表中没有链表项了,更新uxTopReadyPriority*/
    }

    xTimeToWake = xConstTickCount + xTicksToWait;
    listSET_LIST_ITEM_VALUE(&(pxCurrentTCB->xStateListItem), xTimeToWake);

    if (xTimeToWake < xConstTickCount) /*唤醒时间溢出*/
    {
        vListInsert(pxOverflowDelayedTaskList, &(pxCurrentTCB->xStateListItem));
    }
    else /*唤醒时间未溢出*/
    {
        vListInsert(pxDelayedTaskList, &(pxCurrentTCB->xStateListItem));

        if (xTimeToWake < xNextTaskUnblockTime)
        {
            xNextTaskUnblockTime = xTimeToWake; /*唤醒时间溢出意味着在延时任务回到就绪链表之前系统滴答值就会发生溢出，xNextTaskUnblockTime的值
                                                  在系统滴答值发生溢出时调用的taskSWITCH_DELAYED_LISTS宏内更新*/
        }
    }
}

/**
 * @brief 任务延时
 */
void vTaskDelay(const TickType_t xTicksToDelay)
{
    BaseType_t xAlreadyYielded = pdFALSE;

    if (xTicksToDelay > (TickType_t)0U)
    {
        configASSERT(uxSchedulerSuspended == 0);
        vTaskSuspendAll();
        {
            prvAddCurrentTaskToDelayedList(xTicksToDelay);
        }
        xAlreadyYielded = xTaskResumeAll();

        if (xAlreadyYielded == pdFALSE)
        {
            taskYIELD();
        }
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief 递增系统节拍
 */
BaseType_t xTaskIncrementTick(void)
{
    TCB_t *pxTCB;
    TickType_t xItemValue;
    BaseType_t xSwitchRequired = pdFALSE;

    if (uxSchedulerSuspended == (UBaseType_t)pdFALSE)
    {
        const TickType_t xConstTickCount = xTickCount + 1;
        xTickCount = xConstTickCount; /*避免编译器优化问题*/
        if (xConstTickCount == (TickType_t)0U)
        {
            taskSWITCH_DELAYED_LISTS(); /*切换延时链表和溢出延时链表*/
        }
        if (xConstTickCount >= xNextTaskUnblockTime)
        {
            for (;;)
            {
                /*延时链表中已经没有延时任务了,避免重复触发*/
                if (listLIST_IS_EMPTY(pxDelayedTaskList) != pdFALSE)
                {
                    xNextTaskUnblockTime = portMAX_DELAY;
                    break;
                }
                /*还有延时任务没有处理*/
                else
                {
                    pxTCB = (TCB_t *)listGET_OWNER_OF_HEAD_ENTRY(pxDelayedTaskList);
                    xItemValue = listGET_LIST_ITEM_VALUE(&(pxTCB->xStateListItem));

                    if (xConstTickCount < xItemValue)
                    {
                        xNextTaskUnblockTime = xItemValue; /*更新xNextTaskUnblockTime */
                        break;
                    }

                    (void)uxListRemove(&(pxTCB->xStateListItem)); /*两个if都没有触发,也就意味着延时任务到期了*/
                    prvAddTaskToReadyList(pxTCB);

                    if (pxTCB->uxPriority >= pxCurrentTCB->uxPriority)
                    {
                        xSwitchRequired = pdTRUE;
                    }
                }
            }
        }

        if (listCURRENT_LIST_LENGTH(&(pxReadyTasksLists[pxCurrentTCB->uxPriority])) > (UBaseType_t)1)
        {
            xSwitchRequired = pdTRUE;
        }
    }
    else
    {
        ++uxPendedTicks;
    }

    if( xYieldPending != pdFALSE )
	{
		xSwitchRequired = pdTRUE;
	}

    return xSwitchRequired;
}

/**
 * @brief 执行任务切换
 */
void vTaskSwitchContext(void)
{
    if( uxSchedulerSuspended != ( UBaseType_t ) pdFALSE )
	{
		xYieldPending = pdTRUE;
	}
    else
    {
        xYieldPending = pdFALSE;
        taskSELECT_HIGHEST_PRIORITY_TASK();
    }
}
