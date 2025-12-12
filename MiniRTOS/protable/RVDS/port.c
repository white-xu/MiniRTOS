#include "MiniRTOS.h"

/**/
#define portVECTACTIVE_MASK (0xFFUL)	/*ICSR————Interrupt Control and State Register。VECTACTIVE部分(低八位)包含有效的异常编号*/
/*-----------------------------------------------------------*/

/**/
#define portNVIC_SYSTICK_CTRL_REG (*((volatile uint32_t *)0xe000e010))
#define portNVIC_SYSTICK_CLK_BIT (1UL << 2UL)
#define portNVIC_SYSTICK_INT_BIT (1UL << 1UL)
#define portNVIC_SYSTICK_ENABLE_BIT (1UL << 0UL)

#define portNVIC_SYSTICK_LOAD_REG (*((volatile uint32_t *)0xe000e014))
#define configSYSTICK_CLOCK_HZ SystemCoreClock

#define portNVIC_SYSPRI3_REG (*((volatile uint32_t *)0xe000ed20))

#define portNVIC_PENDSV_PRI (((uint32_t)configKERNEL_INTERRUPT_PRIORITY) << 16UL)
#define portNVIC_SYSTICK_PRI (((uint32_t)configKERNEL_INTERRUPT_PRIORITY) << 24UL)
/*-----------------------------------------------------------*/

/**/
#define portFPCCR ((volatile uint32_t *)0xe000ef34) /* Floating point context control register. */
#define portASPEN_AND_LSPEN_BITS (0x3UL << 30UL)
/*-----------------------------------------------------------*/

/*初始化任务栈使用到的常量*/
#define portINITIAL_XPSR (0x01000000)
#define portINITIAL_EXEC_RETURN (0xfffffffd) /*控制控制异常返回的行为(返回时使用PSP、返回到Thread模式、使用浮点寄存器的惰性压栈)*/
#define portSTART_ADDRESS_MASK ((StackType_t)0xfffffffeUL)	/*For strict compliance with the Cortex-M spec the task start address should
															have bit-0 clear, as it is loaded into the PC on exit from an ISR*/
/*-----------------------------------------------------------*/

/**
 * @brief 临界区嵌套计数值
 */
static UBaseType_t uxCriticalNesting = 0xaaaaaaaa;

/**
 * @brief 进入临界区
 */
void vPortEnterCritical(void)
{
	portDISABLE_INTERRUPTS();
	uxCriticalNesting++;

	if (uxCriticalNesting == 1)
	{
		configASSERT((portNVIC_INT_CTRL_REG & portVECTACTIVE_MASK) == 0); /*确保处于线程状态*/
	}
}

/**
 * @brief 退出临界区
 */
void vPortExitCritical(void)
{
	configASSERT(uxCriticalNesting);
	uxCriticalNesting--;
	if (uxCriticalNesting == 0)
	{
		portENABLE_INTERRUPTS();
	}
}

/**
 * @brief 异常返回处理函数
 */
static void prvTaskExitError(void)
{
	configASSERT(uxCriticalNesting == ~0UL);
	portDISABLE_INTERRUPTS();
	for (;;)
		;
}

/**
 * @brief 初始化任务栈
 */
StackType_t *pxPortInitialiseStack(StackType_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters)
{
	pxTopOfStack--;
	*pxTopOfStack = portINITIAL_XPSR; /* xPSR */
	pxTopOfStack--;
	*pxTopOfStack = ((StackType_t)pxCode) & portSTART_ADDRESS_MASK; /* PC */
	pxTopOfStack--;
	*pxTopOfStack = (StackType_t)prvTaskExitError; /* LR */

	pxTopOfStack -= 5;						   /* R12, R3, R2 and R1. */
	*pxTopOfStack = (StackType_t)pvParameters; /* R0 */

	pxTopOfStack--;
	*pxTopOfStack = portINITIAL_EXEC_RETURN;

	pxTopOfStack -= 8; /* R11, R10, R9, R8, R7, R6, R5 and R4. */

	return pxTopOfStack;
}

/**
 * @brief 启动第一个任务
 */
__asm void prvStartFirstTask(void)
{
	PRESERVE8

	ldr r0, = 0xE000ED08 ldr r0, [r0] ldr r0, [r0]

		msr msp,
		r0

			cpsie i		/*开启中断*/
				cpsie f /*开启快速中断*/
					dsb
						isb

							svc 0 /*触发一次SVC异常*/
		nop nop
}

__asm void prvEnableVFP(void)
{
	PRESERVE8

	ldr.w r0, = 0xE000ED88 ldr r1, [r0]

		orr r1,
		  r1, #(0xf << 20) str r1, [r0] bx r14 nop
}

static void vPortSetupTimerInterrupt(void)
{
	/* Configure SysTick to interrupt at the requested rate. */
	portNVIC_SYSTICK_LOAD_REG = (configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ) - 1UL;
	portNVIC_SYSTICK_CTRL_REG = (portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT);
}

/**
 * @brief 启动调度器
 */
BaseType_t xPortStartScheduler(void)
{
	/* Make PendSV and SysTick the lowest priority interrupts. */
	portNVIC_SYSPRI3_REG |= portNVIC_PENDSV_PRI;
	portNVIC_SYSPRI3_REG |= portNVIC_SYSTICK_PRI;

	/* Start the timer that generates the tick ISR.  Interrupts are disabled
	here already. */
	vPortSetupTimerInterrupt();

	/* Initialise the critical nesting count ready for the first task. */
	uxCriticalNesting = 0;

	/* Ensure the VFP is enabled - it should be anyway. */
	prvEnableVFP();

	/* Lazy save always. */
	*(portFPCCR) |= portASPEN_AND_LSPEN_BITS;

	/* Start the first task. */
	prvStartFirstTask();

	/* Should not get here! */
	return 0;
}

/**
 * @brief SVC异常处理函数
 */
__asm void vPortSVCHandler(void)
{
	PRESERVE8

	ldr r3, = pxCurrentTCB
				ldr r1,
		[r3] ldr r0, [r1]

		ldmia r0 !,
		{r4 - r11, r14}

	msr psp,
		r0
			isb
				mov r0,
		#0 msr basepri, r0 bx r14
}

/**
 * @brief 让编译器保持安静
 */
BaseType_t xTaskIncrementTick( void )
{
	return pdTRUE;
}

/**
 * @brief SysTick中断处理函数
 */
void xPortSysTickHandler( void )
{
	vPortRaiseBASEPRI();
	{
		if( xTaskIncrementTick() != pdFALSE )
		{
			portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;
		}
	}
	vPortClearBASEPRIFromISR();
}

/**
 * @brief PendSV异常处理函数
 */
__asm void xPortPendSVHandler( void )
{
	extern uxCriticalNesting;
	extern pxCurrentTCB;
	extern vTaskSwitchContext;

	PRESERVE8

	mrs r0, psp
	isb

	ldr	r3, =pxCurrentTCB
	ldr	r2, [r3]

	/* Is the task using the FPU context?  If so, push high vfp registers. */
	tst r14, #0x10
	it eq
	vstmdbeq r0!, {s16-s31}

	/* Save the core registers. */
	stmdb r0!, {r4-r11, r14}

	/* Save the new top of stack into the first member of the TCB. */
	str r0, [r2]

	stmdb sp!, {r3}

	mov r0, #configMAX_SYSCALL_INTERRUPT_PRIORITY
	msr basepri, r0
	dsb
	isb

	bl vTaskSwitchContext

	mov r0, #0
	msr basepri, r0

	ldmia sp!, {r3}

	/* The first item in pxCurrentTCB is the task top of stack. */
	ldr r1, [r3]
	ldr r0, [r1]

	/* Pop the core registers. */
	ldmia r0!, {r4-r11, r14}

	/* Is the task using the FPU context?  If so, pop the high vfp registers
	too. */
	tst r14, #0x10
	it eq
	vldmiaeq r0!, {s16-s31}

	msr psp, r0
	isb

	bx r14
}
