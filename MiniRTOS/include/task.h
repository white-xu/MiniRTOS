#ifndef INC_TASK_H
#define INC_TASK_H

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void * TaskHandle_t;

/*
 * Used internally only.
 */
typedef struct xTIME_OUT
{
	BaseType_t xOverflowCount;
	TickType_t xTimeOnEntering;
} TimeOut_t;

#define tskIDLE_PRIORITY			( ( UBaseType_t ) 0U )

#define taskYIELD()					portYIELD()

#define taskENTER_CRITICAL()		portENTER_CRITICAL()
#define taskEXIT_CRITICAL()			portEXIT_CRITICAL()
#define taskENTER_CRITICAL_FROM_ISR() portSET_INTERRUPT_MASK_FROM_ISR()
#define taskEXIT_CRITICAL_FROM_ISR( x ) portCLEAR_INTERRUPT_MASK_FROM_ISR( x )

#define taskDISABLE_INTERRUPTS()	portDISABLE_INTERRUPTS()
#define taskENABLE_INTERRUPTS()		portENABLE_INTERRUPTS()

BaseType_t xTaskCreate(TaskFunction_t pxTaskCode,
                       const char *const pcName,
                       const uint16_t usStackDepth,
                       void *const pvParameters,
                       UBaseType_t uxPriority,
                       TaskHandle_t *const pxCreatedTask);
void vTaskStartScheduler(void);
BaseType_t xTaskIncrementTick(void);
void vTaskSwitchContext(void);
void vTaskDelay(const TickType_t xTicksToDelay);
void vTaskSuspendAll(void);
BaseType_t xTaskResumeAll(void);

BaseType_t xTaskRemoveFromEventList( const List_t * const pxEventList );                            /*暂时未实现(避免编译器报错)*/
void vTaskSetTimeOutState( TimeOut_t * const pxTimeOut );                                           /*暂时未实现(避免编译器报错)*/
BaseType_t xTaskCheckForTimeOut( TimeOut_t * const pxTimeOut, TickType_t * const pxTicksToWait );   /*暂时未实现(避免编译器报错)*/
void vTaskPlaceOnEventList( List_t * const pxEventList, const TickType_t xTicksToWait );            /*暂时未实现(避免编译器报错)*/
void vTaskMissedYield( void );

#ifdef __cplusplus
}
#endif

#endif
