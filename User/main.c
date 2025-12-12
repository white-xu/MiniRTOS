#include "main.h"

static void SystemClock_Config(void);

#define TASK1_TASK_PRIO 2
#define TASK1_STK_SIZE 560
TaskHandle_t Task1Task_Handler;
static void task1_task(void *pvParameters);

#define TASK2_TASK_PRIO 2
#define TASK2_STK_SIZE 560
TaskHandle_t Task2Task_Handler;
static void task2_task(void *pvParameters);

/**
 * @brief main
 */
int main(void)
{
	HAL_Init();

	SystemClock_Config();

	CPU_TS_TmrInit();
	DEBUG_USART_Config();
	
	cm_backtrace_init("MiniRTOS_Debug", "V1.0.0", "V1.0.0");

	printf("print");
	printf("print");
	printf("print");
	printf("print");

	BaseType_t ret;
	ret = xTaskCreate((TaskFunction_t )task1_task,
	            	(const char*    )"task1_task",
	            	(uint16_t       )TASK1_STK_SIZE,
	            	(void*          )NULL,
	            	(UBaseType_t    )TASK1_TASK_PRIO,
	            	(TaskHandle_t*  )&Task1Task_Handler);
	if(ret == pdPASS)
	{
		PRINT_INFO("task1 create successfully");
	}
	else
	{
		PRINT_INFO("task1 create failed");
	}

	ret = xTaskCreate((TaskFunction_t )task2_task,
	            	(const char*    )"task2_task",
	            	(uint16_t       )TASK2_STK_SIZE,
	            	(void*          )NULL,
	            	(UBaseType_t    )TASK2_TASK_PRIO,
	            	(TaskHandle_t*  )&Task2Task_Handler);
	if(ret == pdPASS)
	{
		PRINT_INFO("task2 create successfully");
	}
	else
	{
		PRINT_INFO("task2 create failed");
	}

	vTaskStartScheduler();

	while (1)
	{
		PRINT_INFO("裸机运行没问题");
		Delay_ms(1000);
		Delay_ms(1000);
	}
}

static void task1_task(void *pvParameters)
{
    static uint32_t count1 = 0;
	const uint32_t print_interval = 80000000;	/*大概6s的延时*/

	PRINT_INFO("enter task1");

	while (1)
	{
		count1++;

		if (count1 >= print_interval)
		{
			PRINT_INFO("task1:%lu (total loops)", count1);
			count1 = 0;
		}
	}
}

static void task2_task(void *pvParameters)
{
    static uint32_t count2 = 0;
	const uint32_t print_interval = 150000000;

	PRINT_INFO("enter task2");

	while (1)
	{
		count2++;

		if (count2 >= print_interval)
		{
			PRINT_INFO("task2:%lu (total loops)", count2);
			count2 = 0;
		}
	}
}

/**
 * @brief  System Clock Configuration
 *         The system Clock is configured as follow :
 *            System Clock source            = PLL (HSE)
 *            SYSCLK(Hz)                     = 168000000
 *            HCLK(Hz)                       = 168000000
 *            AHB Prescaler                  = 1
 *            APB1 Prescaler                 = 4
 *            APB2 Prescaler                 = 2
 *            HSE Frequency(Hz)              = 8000000
 *            PLL_M                          = 25
 *            PLL_N                          = 336
 *            PLL_P                          = 2
 *            PLL_Q                          = 7
 *            VDD(V)                         = 3.3
 *            Main regulator output voltage  = Scale1 mode
 *            Flash Latency(WS)              = 5
 * @param  None
 * @retval None
 */
static void SystemClock_Config(void)
{
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_OscInitTypeDef RCC_OscInitStruct;

	/* Enable Power Control clock */
	__HAL_RCC_PWR_CLK_ENABLE();

	/* The voltage scaling allows optimizing the power consumption when the device is
	   clocked below the maximum system frequency, to update the voltage scaling value
	   regarding system frequency refer to product datasheet.  */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/* Enable HSE Oscillator and activate PLL with HSE as source */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 25;
	RCC_OscInitStruct.PLL.PLLN = 336;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 7;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		while (1)
		{
		};
	}

	/* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
	   clocks dividers */
	RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
	{
		while (1)
		{
		};
	}

	/* STM32F405x/407x/415x/417x Revision Z devices: prefetch is supported  */
	if (HAL_GetREVID() == 0x1001)
	{
		/* Enable the Flash prefetch */
		__HAL_FLASH_PREFETCH_BUFFER_ENABLE();
	}
}
