/**
 ******************************************************************************
 * @file    system_stm32f4xx.c
 * @author  MCD Application Team
 * @brief   CMSIS Cortex-M4 Device Peripheral Access Layer System Source File.
 *
 *   This file provides two functions and one global variable to be called from
 *   user application:
 *      - SystemInit(): This function is called at startup just after reset and
 *                      before branch to main program. This call is made inside
 *                      the "startup_stm32f4xx.s" file.
 *
 *      - SystemCoreClock variable: Contains the core clock (HCLK), it can be used
 *                                  by the user application to setup the SysTick
 *                                  timer or configure other parameters.
 *
 *      - SystemCoreClockUpdate(): Updates the variable SystemCoreClock and must
 *                                 be called whenever the core clock is changed
 *                                 during program execution.
 *
 *
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2017 STMicroelectronics </center></h2>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

/** @addtogroup CMSIS
 * @{
 */

/** @addtogroup stm32f4xx_system
 * @{
 */

/** @addtogroup STM32F4xx_System_Private_Includes
 * @{
 */

#include "stm32f4xx.h"

#if !defined(HSE_VALUE)
#define HSE_VALUE ((uint32_t)8000000) /*!< Default value of the External oscillator in Hz */
#endif								  /* HSE_VALUE */

#if !defined(HSI_VALUE)
#define HSI_VALUE ((uint32_t)16000000) /*!< Value of the Internal oscillator in Hz*/
#endif								   /* HSI_VALUE */

/**
 * @}
 */

/** @addtogroup STM32F4xx_System_Private_TypesDefinitions
 * @{
 */

/**
 * @}
 */

/** @addtogroup STM32F4xx_System_Private_Defines
 * @{
 */

/************************* Miscellaneous Configuration ************************/
/*!< Uncomment the following line if you need to use external SDRAM mounted
	 on DK as data memory  */
/* #define DATA_IN_ExtSDRAM */

/*!< Uncomment the following line if you need to relocate your vector Table in
	 Internal SRAM. */
/* #define VECT_TAB_SRAM */
#define VECT_TAB_OFFSET 0x00 /*!< Vector Table base offset field. \
								  This value must be a multiple of 0x200. */
/******************************************************************************/

/**
 * @}
 */

/** @addtogroup STM32F4xx_System_Private_Macros
 * @{
 */

/**
 * @}
 */

/** @addtogroup STM32F4xx_System_Private_Variables
 * @{
 */
/* This variable is updated in three ways:
	1) by calling CMSIS function SystemCoreClockUpdate()
	2) by calling HAL API function HAL_RCC_GetHCLKFreq()
	3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency
	   Note: If you use this function to configure the system clock; then there
			 is no need to call the 2 first functions listed above, since SystemCoreClock
			 variable is updated automatically.
*/
uint32_t SystemCoreClock = 16000000;
const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t APBPrescTable[8] = {0, 0, 0, 0, 1, 2, 3, 4};
/**
 * @}
 */

/** @addtogroup STM32F4xx_System_Private_FunctionPrototypes
 * @{
 */
#if defined(DATA_IN_ExtSDRAM)
static void SystemInit_ExtMemCtl(void);
#endif /* DATA_IN_ExtSDRAM */

/** @addtogroup STM32F4xx_System_Private_FunctionPrototypes
 * @{
 */

/**
 * @}
 */

/** @addtogroup STM32F4xx_System_Private_Functions
 * @{
 */

/**
 * @brief  Setup the microcontroller system
 *         Initialize the FPU setting, vector table location and External memory
 *         configuration.
 * @param  None
 * @retval None
 */
void SystemInit(void)
{
/* FPU settings ------------------------------------------------------------*/
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
	SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2)); /* set CP10 and CP11 Full Access */
#endif

	/***************************将RCC时钟配置复位到默认状态***********************************/
	/* Set HSION bit - 启用内部高速时钟(HSI) */
	RCC->CR |= (uint32_t)0x00000001; /* 第0位置1：HSI振荡器使能 */
	/* 作用：确保系统至少有内部8MHz（或不同型号的具体频率）时钟源运行，
	   这是最基本的系统时钟，即使外部晶振失效也能保证芯片运行 */

	/* Reset CFGR register - 复位时钟配置寄存器 */
	RCC->CFGR = 0x00000000; /* 清零所有位 */
	/* 作用：清除所有时钟相关配置，包括：
	   - 系统时钟源选择（SW[1:0]）
	   - AHB、APB1、APB2预分频器
	   - MCO时钟输出配置
	   - PLL配置相关位
	   复位后默认使用HSI作为系统时钟，所有分频器=1 */

	/* Reset HSEON, CSSON and PLLON bits - 关闭外部时钟、时钟安全系统和PLL */
	RCC->CR &= (uint32_t)0xFEF6FFFF; /* 清除第16位(HSEON)、第19位(CSSON)、第24位(PLLON) */
	/* 作用：
	   - 关闭外部高速时钟(HSE)，停止外部晶振
	   - 关闭时钟安全系统(CSS)，该功能用于检测HSE故障
	   - 关闭PLL锁相环，停止倍频
	   这样可以降低功耗并避免从错误状态启动 */

	/* Reset PLLCFGR register - 复位PLL配置寄存器 */
	RCC->PLLCFGR = 0x24003010; /* 写入复位默认值 */
	/* 复位值0x24003010的含义：
	   - PLLM[5:0] = 0x10 (16): 输入分频系数
	   - PLLN[8:0] = 0x30 (48): VCO倍频系数
	   - PLLP[1:0] = 0x0   (2): PLL时钟输出分频（主系统时钟）
	   - PLLSRC = 0: PLL时钟源选择HSI
	   - PLLQ[3:0] = 0x4  (4): USB OTG FS/SDIO/RNG时钟分频
	   这是STM32F4系列默认的PLL配置 */

	/* Reset HSEBYP bit - 清除外部晶振旁路位 */
	RCC->CR &= (uint32_t)0xFFFBFFFF; /* 清除第18位(HSEBYP) */
	/* 作用：禁用外部时钟旁路模式
	   - 0: 使用外部晶振/陶瓷谐振器（需要起振电路）
	   - 1: 旁路模式，直接输入外部时钟信号（如来自有源晶振）
	   复位后使用正常晶振模式 */

	/* Disable all interrupts - 禁用所有时钟相关中断 */
	RCC->CIR = 0x00000000; /* 清零时钟中断寄存器 */
	/* 作用：清除所有时钟相关的中断标志位和中断使能位，包括：
	   - 就绪中断（HSIRDY、LSIRDY、HSERDY、LSERDY、PLLRDY）
	   - 时钟安全系统中断（CSS）
	   - 清除对应的中断挂起标志
	   防止在初始化过程中意外触发中断 */

#if defined(DATA_IN_ExtSDRAM)
	SystemInit_ExtMemCtl();
#endif /* DATA_IN_ExtSDRAM */

	/* Configure the Vector Table location add offset address ------------------*/
#ifdef VECT_TAB_SRAM
	SCB->VTOR = SRAM_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal SRAM */
#else
	SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal FLASH */
#endif
}

/**
 * @brief  Update SystemCoreClock variable according to Clock Register Values.
 *         The SystemCoreClock variable contains the core clock (HCLK), it can
 *         be used by the user application to setup the SysTick timer or configure
 *         other parameters.
 *
 * @note   Each time the core clock (HCLK) changes, this function must be called
 *         to update SystemCoreClock variable value. Otherwise, any configuration
 *         based on this variable will be incorrect.
 *
 * @note   - The system frequency computed by this function is not the real
 *           frequency in the chip. It is calculated based on the predefined
 *           constant and the selected clock source:
 *
 *           - If SYSCLK source is HSI, SystemCoreClock will contain the HSI_VALUE(*)
 *
 *           - If SYSCLK source is HSE, SystemCoreClock will contain the HSE_VALUE(**)
 *
 *           - If SYSCLK source is PLL, SystemCoreClock will contain the HSE_VALUE(**)
 *             or HSI_VALUE(*) multiplied/divided by the PLL factors.
 *
 *         (*) HSI_VALUE is a constant defined in stm32f4xx_hal_conf.h file (default value
 *             16 MHz) but the real value may vary depending on the variations
 *             in voltage and temperature.
 *
 *         (**) HSE_VALUE is a constant defined in stm32f4xx_hal_conf.h file (its value
 *              depends on the application requirements), user has to ensure that HSE_VALUE
 *              is same as the real frequency of the crystal used. Otherwise, this function
 *              may have wrong result.
 *
 *         - The result of this function could be not correct when using fractional
 *           value for HSE crystal.
 *
 * @param  None
 * @retval None
 */
void SystemCoreClockUpdate(void)
{
	uint32_t tmp = 0, pllvco = 0, pllp = 2, pllsource = 0, pllm = 2;

	/* Get SYSCLK source -------------------------------------------------------*/
	tmp = RCC->CFGR & RCC_CFGR_SWS;

	switch (tmp)
	{
	case 0x00: /* HSI used as system clock source */
		SystemCoreClock = HSI_VALUE;
		break;
	case 0x04: /* HSE used as system clock source */
		SystemCoreClock = HSE_VALUE;
		break;
	case 0x08: /* PLL used as system clock source */

		/* PLL_VCO = (HSE_VALUE or HSI_VALUE / PLL_M) * PLL_N
		   SYSCLK = PLL_VCO / PLL_P
		   */
		pllsource = (RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC) >> 22;
		pllm = RCC->PLLCFGR & RCC_PLLCFGR_PLLM;

		if (pllsource != 0)
		{
			/* HSE used as PLL clock source */
			pllvco = (HSE_VALUE / pllm) * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> 6);
		}
		else
		{
			/* HSI used as PLL clock source */
			pllvco = (HSI_VALUE / pllm) * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> 6);
		}

		pllp = (((RCC->PLLCFGR & RCC_PLLCFGR_PLLP) >> 16) + 1) * 2;
		SystemCoreClock = pllvco / pllp;
		break;
	default:
		SystemCoreClock = HSI_VALUE;
		break;
	}
	/* Compute HCLK frequency --------------------------------------------------*/
	/* Get HCLK prescaler */
	tmp = AHBPrescTable[((RCC->CFGR & RCC_CFGR_HPRE) >> 4)];
	/* HCLK frequency */
	SystemCoreClock >>= tmp;
}

#if defined(DATA_IN_ExtSDRAM)
/**
 * @brief  Setup the external memory controller.
 *         Called in startup_stm32f4xx.s before jump to main.
 *         This function configures the external memories (SDRAM)
 *         This SDRAM will be used as program data memory (including heap and stack).
 * @param  None
 * @retval None
 */
void SystemInit_ExtMemCtl(void)
{
	register uint32_t tmpreg = 0, timeout = 0xFFFF;
	register __IO uint32_t index;

	/* Enable GPIOB, GPIOC, GPIOD, GPIOE, GPIOF and GPIOG interface
	clock */
	RCC->AHB1ENR |= 0x0000007E;

	/* Connect PBx pins to FMC Alternate function */
	GPIOB->AFR[0] = 0x0CC00000;
	GPIOB->AFR[1] = 0x00000000;
	/* Configure PBx pins in Alternate function mode */
	GPIOB->MODER = 0x00002A80;
	/* Configure PBx pins speed to 100 MHz */
	GPIOB->OSPEEDR = 0x00003CC0;
	/* Configure PBx pins Output type to push-pull */
	GPIOB->OTYPER = 0x00000000;
	/* No pull-up, pull-down for PBx pins */
	GPIOB->PUPDR = 0x00000100;

	/* Connect PCx pins to FMC Alternate function */
	GPIOC->AFR[0] = 0x0000000C;
	GPIOC->AFR[1] = 0x00000000;
	/* Configure PCx pins in Alternate function mode */
	GPIOC->MODER = 0x00000002;
	/* Configure PCx pins speed to 100 MHz */
	GPIOC->OSPEEDR = 0x00000003;
	/* Configure PCx pins Output type to push-pull */
	GPIOC->OTYPER = 0x00000000;
	/* No pull-up, pull-down for PCx pins */
	GPIOC->PUPDR = 0x00000000;

	/* Connect PDx pins to FMC Alternate function */
	GPIOD->AFR[0] = 0x000000CC;
	GPIOD->AFR[1] = 0xCC000CCC;
	/* Configure PDx pins in Alternate function mode */
	GPIOD->MODER = 0xA02A000A;
	/* Configure PDx pins speed to 100 MHz */
	GPIOD->OSPEEDR = 0xF03F000F;
	/* Configure PDx pins Output type to push-pull */
	GPIOD->OTYPER = 0x00000000;
	/* No pull-up, pull-down for PDx pins */
	GPIOD->PUPDR = 0x00000000;

	/* Connect PEx pins to FMC Alternate function */
	GPIOE->AFR[0] = 0xC00000CC;
	GPIOE->AFR[1] = 0xCCCCCCCC;
	/* Configure PEx pins in Alternate function mode */
	GPIOE->MODER = 0xAAAA800A;
	/* Configure PEx pins speed to 100 MHz */
	GPIOE->OSPEEDR = 0xFFFFC00F;
	/* Configure PEx pins Output type to push-pull */
	GPIOE->OTYPER = 0x00000000;
	/* No pull-up, pull-down for PEx pins */
	GPIOE->PUPDR = 0x00000000;

	/* Connect PFx pins to FMC Alternate function */
	GPIOF->AFR[0] = 0x00CCCCCC;
	GPIOF->AFR[1] = 0xCCCCC000;
	/* Configure PFx pins in Alternate function mode */
	GPIOF->MODER = 0xAA800AAA;
	/* Configure PFx pins speed to 100 MHz */
	GPIOF->OSPEEDR = 0xFFC00FFF;
	/* Configure PFx pins Output type to push-pull */
	GPIOF->OTYPER = 0x00000000;
	/* No pull-up, pull-down for PFx pins */
	GPIOF->PUPDR = 0x00000000;

	/* Connect PGx pins to FMC Alternate function */
	GPIOG->AFR[0] = 0x00CC00CC;
	GPIOG->AFR[1] = 0xC000000C;
	/* Configure PGx pins in Alternate function mode */
	GPIOG->MODER = 0x80020A0A;
	/* Configure PGx pins speed to 100 MHz */
	GPIOG->OSPEEDR = 0xC0030F0F;
	/* Configure PGx pins Output type to push-pull */
	GPIOG->OTYPER = 0x00000000;
	/* No pull-up, pull-down for PGx pins */
	GPIOG->PUPDR = 0x00000000;

	/* FMC Configuration */
	/* Enable the FMC interface clock */
	RCC->AHB3ENR |= 0x00000001;

	/* Configure and enable SDRAM bank2 */
	FMC_Bank5_6->SDCR[0] = 0x00002ED0;
	FMC_Bank5_6->SDCR[1] = 0x000001D4;
	FMC_Bank5_6->SDTR[0] = 0x0F1F6FFF;
	FMC_Bank5_6->SDTR[1] = 0x01010361;

	/* SDRAM initialization sequence */
	/* Clock enable command */
	FMC_Bank5_6->SDCMR = 0x00000009;
	tmpreg = FMC_Bank5_6->SDSR & 0x00000020;
	while ((tmpreg != 0) && (timeout-- > 0))
	{
		tmpreg = FMC_Bank5_6->SDSR & 0x00000020;
	}

	/* Delay */
	for (index = 0; index < 1000; index++)
		;

	/* PALL command */
	FMC_Bank5_6->SDCMR = 0x0000000A;
	timeout = 0xFFFF;
	while ((tmpreg != 0) && (timeout-- > 0))
	{
		tmpreg = FMC_Bank5_6->SDSR & 0x00000020;
	}

	/* Auto refresh command */
	FMC_Bank5_6->SDCMR = 0x0000006B;
	timeout = 0xFFFF;
	while ((tmpreg != 0) && (timeout-- > 0))
	{
		tmpreg = FMC_Bank5_6->SDSR & 0x00000020;
	}

	/* MRD register program */
	FMC_Bank5_6->SDCMR = 0x0004620C;
	timeout = 0xFFFF;
	while ((tmpreg != 0) && (timeout-- > 0))
	{
		tmpreg = FMC_Bank5_6->SDSR & 0x00000020;
	}

	/* Set refresh count */
	tmpreg = FMC_Bank5_6->SDRTR;
	FMC_Bank5_6->SDRTR = (tmpreg | (0x0000056A << 1));

	/* Disable write protection */
	tmpreg = FMC_Bank5_6->SDCR[1];
	FMC_Bank5_6->SDCR[1] = (tmpreg & 0xFFFFFDFF);
}
#endif /* DATA_IN_ExtSDRAM */

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
