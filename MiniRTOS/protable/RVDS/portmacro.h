#ifndef PORTMACRO_H
#define PORTMACRO_H

#ifdef __cplusplus
extern "C" {
#endif

/*类型定义*/
#define portCHAR		char
#define portFLOAT		float
#define portDOUBLE		double
#define portLONG		long
#define portSHORT		short
#define portSTACK_TYPE	uint32_t
#define portBASE_TYPE	long

typedef portSTACK_TYPE StackType_t;		/*uint32_t*/
typedef long BaseType_t;				/*long*/
typedef unsigned long UBaseType_t;		/*unsigned long*/
typedef uint32_t TickType_t;			/*uint32_t*/

#define portMAX_DELAY ( TickType_t ) 0xffffffffUL
/*-----------------------------------------------------------*/

/*临界区管理*/
extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );

#define portDISABLE_INTERRUPTS()				vPortRaiseBASEPRI()
#define portENABLE_INTERRUPTS()					vPortSetBASEPRI( 0 )
#define portENTER_CRITICAL()					vPortEnterCritical()
#define portEXIT_CRITICAL()						vPortExitCritical()
#define portSET_INTERRUPT_MASK_FROM_ISR()		ulPortRaiseBASEPRI()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(x)	vPortSetBASEPRI(x)
/*-----------------------------------------------------------*/

/*PendSV异常相关*/
#define portSY_FULL_READ_WRITE		( 15 )	/*与内存屏障内联函数相关的常量*/
#define portNVIC_INT_CTRL_REG		( * ( ( volatile uint32_t * ) 0xe000ed04 ) )
#define portNVIC_PENDSVSET_BIT		( 1UL << 28UL )
#define portYIELD()																\
{																				\
	portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;								\
																				\
	__dsb( portSY_FULL_READ_WRITE );											\
	__isb( portSY_FULL_READ_WRITE );											\
}
#define portEND_SWITCHING_ISR( xSwitchRequired ) if( xSwitchRequired != pdFALSE ) portYIELD()
#define portYIELD_FROM_ISR( x ) portEND_SWITCHING_ISR( x )
/*-----------------------------------------------------------*/

/*中断优先级配置*/
#define configPRIO_BITS       		                    4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY			15     
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY	5 
#define configKERNEL_INTERRUPT_PRIORITY 		( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
/*-----------------------------------------------------------*/

/*就绪任务优先级位图操作*/
#define portRECORD_READY_PRIORITY( uxPriority, uxReadyPriorities ) ( uxReadyPriorities ) |= ( 1UL << ( uxPriority ) )
#define portRESET_READY_PRIORITY( uxPriority, uxReadyPriorities ) ( uxReadyPriorities ) &= ~( 1UL << ( uxPriority ) )
#define portGET_HIGHEST_PRIORITY( uxTopPriority, uxReadyPriorities ) uxTopPriority = ( 31UL - ( uint32_t ) __clz( ( uxReadyPriorities ) ) )
/*-----------------------------------------------------------*/

/*
 *内核寄存器BASEPRI——Base Priority Mask Register.BASEPRI寄存器定义了异常处理的最低优先级。
 *当BASEPRI被设置为非零值时，它会阻止激活所有(逻辑)优先级与BASEPRI值相同或更低的异常
 *
 * 内核寄存器IPSR——Interrupt Program Status Register.IPSR寄存器包含了当前正在执行的中断服
 * 务程序（ISR）的异常类型编号
*/
static __forceinline void vPortSetBASEPRI( uint32_t ulBASEPRI )
{
	__asm
	{
		msr basepri, ulBASEPRI
	}
}

static __forceinline void vPortRaiseBASEPRI( void )
{
    uint32_t ulNewBASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY;

	__asm
	{
		msr basepri, ulNewBASEPRI
		dsb
		isb
	}
}

static __forceinline void vPortClearBASEPRIFromISR( void )
{
	__asm
	{
		msr basepri, #0
	}
}

static __forceinline uint32_t ulPortRaiseBASEPRI( void )
{
    uint32_t ulReturn, ulNewBASEPRI = configMAX_SYSCALL_INTERRUPT_PRIORITY;

	__asm
	{
		mrs ulReturn, basepri
		msr basepri, ulNewBASEPRI
		dsb
		isb
	}

	return ulReturn;
}

static __forceinline BaseType_t xPortIsInsideInterrupt( void )
{
    uint32_t ulCurrentInterrupt;
    BaseType_t xReturn;

	__asm
	{
		mrs ulCurrentInterrupt, ipsr
	}

	if( ulCurrentInterrupt == 0 )
	{
		xReturn = pdFALSE;
	}
	else
	{
		xReturn = pdTRUE;
	}

	return xReturn;
}

#ifdef __cplusplus
}
#endif

#endif
