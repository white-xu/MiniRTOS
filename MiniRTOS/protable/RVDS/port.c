#include "stm32f4xx.h"

#include "MiniRTOS.h"
#include "task.h"

/**/
#define portVECTACTIVE_MASK (0xFFUL)	/*内核外设寄存器ICSR—Interrupt Control and State Register低八位掩码(低八位包含有效的异常编号)*/
/*-----------------------------------------------------------*/

/**/
#define portNVIC_SYSTICK_CTRL_REG (*((volatile uint32_t *)0xe000e010))
#define portNVIC_SYSTICK_CLK_BIT (1UL << 2UL)								/*选择处理器时钟*/
#define portNVIC_SYSTICK_INT_BIT (1UL << 1UL)								/*使能中断*/
#define portNVIC_SYSTICK_ENABLE_BIT (1UL << 0UL)							/*使能计数*/

#define portNVIC_SYSTICK_LOAD_REG (*((volatile uint32_t *)0xe000e014))
#define configSYSTICK_CLOCK_HZ SystemCoreClock

#define portNVIC_SYSPRI3_REG (*((volatile uint32_t *)0xe000ed20))

#define portNVIC_PENDSV_PRI (((uint32_t)configKERNEL_INTERRUPT_PRIORITY) << 16UL)
#define portNVIC_SYSTICK_PRI (((uint32_t)configKERNEL_INTERRUPT_PRIORITY) << 24UL)
/*-----------------------------------------------------------*/

/*FPU控制相关寄存器宏定义*/
#define portFPCCR ((volatile uint32_t *)0xe000ef34) /*内核外设寄存器FPCCR——Floating point context control register. 用于设置或返回浮点单元（FPU）的控制数据*/
#define portASPEN_AND_LSPEN_BITS (0x3UL << 30UL)	/*同时启用自动状态保存(ASPEN)和懒保存优化(LSPEN),硬件自动管理FPU寄存器保存与恢复*/
/*-----------------------------------------------------------*/

/*初始化任务栈使用到的常量*/
#define portINITIAL_XPSR (0x01000000)						/*表示处理器处于Thumb状态*/
#define portINITIAL_EXEC_RETURN (0xfffffffd) 				/*从异常返回时加载到LR
                                                       		 *位3=1: 使用PSP
                                                             *位2=0: 返回到Thread模式  
                                                             *位1=1: 从Thumb状态返回
                                                             *位0=1: 使用扩展栈帧(含FPU寄存器)*/
#define portSTART_ADDRESS_MASK ((StackType_t)0xfffffffeUL)	/*为了严格遵循Cortex-M架构规范，任务起始地址的bit-0应该清零*/
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
 * @brief 任务异常返回处理函数
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
	*pxTopOfStack = portINITIAL_XPSR; 								/*xPSR*/
	pxTopOfStack--;
	*pxTopOfStack = ((StackType_t)pxCode) & portSTART_ADDRESS_MASK; /*PC*/
	pxTopOfStack--;
	*pxTopOfStack = (StackType_t)prvTaskExitError; 					/*LR*/

	pxTopOfStack -= 5;						   						/*R12, R3, R2 and R1*/
	*pxTopOfStack = (StackType_t)pvParameters; 						/*R0*/

	pxTopOfStack--;
	*pxTopOfStack = portINITIAL_EXEC_RETURN;

	pxTopOfStack -= 8; 												/*R11, R10, R9, R8, R7, R6, R5 and R4*/

	return pxTopOfStack;
}

/**
 * @brief 启动第一个任务
 */
__asm void prvStartFirstTask( void )
{
	PRESERVE8

	ldr r0, =0xE000ED08		/*内核外设寄存器VTOR——Vector Table Offset Register.存储中断向量表的基地址和内存地址0x00000000的偏移量*/
	ldr r0, [r0]
	ldr r0, [r0]
	msr msp, r0

	cpsie i					/*开启中断*/
	cpsie f					/*开启快速中断*/
	dsb
	isb

	svc 0
	nop
	nop
}

/**
 * @brief 启用浮点协处理器(FPU)的完全访问权限
 */
__asm void prvEnableVFP(void)
{
	PRESERVE8

	ldr.w r0, =0xE000ED88			/*内核外设寄存器CPACR——Coprocessor Access Control Register.用于指定协处理器的访问权限*/
									/*.w 表示宽指令格式*/
	ldr	r1, [r0]

	orr	r1, r1, #( 0xf << 20 )		/*按位或操作，设置第20-23位为1*/
	str r1, [r0]					/*将修改后的值写回 CPACR 寄存器*/
	bx	r14
	nop
}

/**
 * @brief 配置SysTick定时器中断作为FreeRTOS的系统时钟节拍源
 */
static void vPortSetupTimerInterrupt(void)
{
	portNVIC_SYSTICK_LOAD_REG = (configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ) - 1UL;
	portNVIC_SYSTICK_CTRL_REG = (portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT);
}

/**
 * @brief 启动调度器
 */
BaseType_t xPortStartScheduler(void)
{
	/*设置PendSV、Systick优先级为最低优先级*/
	portNVIC_SYSPRI3_REG |= portNVIC_PENDSV_PRI;
	portNVIC_SYSPRI3_REG |= portNVIC_SYSTICK_PRI;

	vPortSetupTimerInterrupt();

	uxCriticalNesting = 0;

	prvEnableVFP();

	*(portFPCCR) |= portASPEN_AND_LSPEN_BITS;

	prvStartFirstTask();

	/*不应该运行到这里*/
	return 0;
}

/**
 * @brief SVC异常处理函数
 */
__asm void vPortSVCHandler(void)
{
	PRESERVE8

	/*加载当前任务控制块地址*/
	ldr	r3, =pxCurrentTCB
	ldr r1, [r3]
	ldr r0, [r1]

	/*从任务栈中恢复调用者保存的寄存器*/
	ldmia r0!, {r4-r11, r14}	

	/*设置进程栈指针为新任务栈顶*/
	msr psp, r0

	isb
	mov r0, #0
	msr	basepri, r0		/*清零BASEPRI寄存器，使能所有中断*/
	bx r14				/*异常返回，硬件自动弹出剩余寄存器并切换到任务模式*/
}

/**
 * @brief SysTick中断处理函数
 */
void xPortSysTickHandler( void )
{
	vPortRaiseBASEPRI();										/*屏蔽逻辑低优先级中断*/
	{
		if( xTaskIncrementTick() != pdFALSE )
		{
			portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;
		}
	}
	vPortClearBASEPRIFromISR();									/*清除屏蔽*/
}

/*
*mrs:将特殊寄存器的值读入通用寄存器
*ldr:加载数据
*tst:测试位
*it:if-then
*vstmdbeq:存储多个FPU寄存器(地址先减)
*stmdb:存储寄存器(地址先减)
*str:值存储到指定内存
*/
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

	tst r14, #0x10										/*在异常处理中，LR(R14)包含EXC_RETURN值:
                         								bit4=0:进入异常前使用了FPU，需手动保存S16-S31
                         								bit4=1:未使用FPU，无需额外操作*/
	it eq
	vstmdbeq r0!, {s16-s31}

	stmdb r0!, {r4-r11, r14}

	str r0, [r2]										/*更新切换前的任务栈指针*/

	stmdb sp!, {r3}										/*r3属于调用者保存寄存器，后面调用了vTaskSwitchContext可能
														  更新pxCurrentTCB的值,所以需要保存*/

	mov r0, #configMAX_SYSCALL_INTERRUPT_PRIORITY
	msr basepri, r0
	dsb
	isb

	bl vTaskSwitchContext

	mov r0, #0
	msr basepri, r0

	ldmia sp!, {r3}

	ldr r1, [r3]										/*r3保存的是pxCurrentTCB的地址。vTaskSwitchContext调用返回后
														  通过ldr r1, [r3]和ldr r0, [r1]两条指令获取即将运行任务的任务栈*/
	ldr r0, [r1]

	ldmia r0!, {r4-r11, r14}

	tst r14, #0x10
	it eq
	vldmiaeq r0!, {s16-s31}

	msr psp, r0
	isb

	bx r14
}
