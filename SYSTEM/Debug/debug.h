/**
 * @file      debug.h
 * @brief     Provide information printing function
 * @author    white-xu
 * @date      2025-10-16
 * @version   1.0.0
 * @note      无
 */

#ifndef __DEBUG_H
#define __DEBUG_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef PRINT_DEBUG_ENABLE
#define PRINT_DEBUG_ENABLE 1 /* 打印调试信息 */
#endif

#ifndef PRINT_ERR_ENABLE
#define PRINT_ERR_ENABLE 1 /* 打印错误信息 */
#endif

#ifndef PRINT_INFO_ENABLE
#define PRINT_INFO_ENABLE 1 /* 打印信息 */
#endif

#ifndef PRINT_LOG_WITH_SOURCE_INFO
#define PRINT_LOG_WITH_SOURCE_INFO 0 /* 显示文件名、行号 */
#endif

#if PRINT_LOG_WITH_SOURCE_INFO

#if PRINT_DEBUG_ENABLE
#define PRINT_DEBUG(fmt, args...)                             \
	do                                                        \
	{                                                         \
		printf("\r\n[DEBUG][%s:%d] >> ", __FILE__, __LINE__); \
		printf(fmt, ##args);                                  \
	} while (0)
#else
#define PRINT_DEBUG(fmt, args...)
#endif

#if PRINT_ERR_ENABLE
#define PRINT_ERR(fmt, args...)                             \
	do                                                      \
	{                                                       \
		printf("\r\n[ERR][%s:%d] >> ", __FILE__, __LINE__); \
		printf(fmt, ##args);                                \
	} while (0)
#else
#define PRINT_ERR(fmt, args...)
#endif

#if PRINT_INFO_ENABLE
#define PRINT_INFO(fmt, args...)                             \
	do                                                       \
	{                                                        \
		printf("\r\n[INFO][%s:%d] >> ", __FILE__, __LINE__); \
		printf(fmt, ##args);                                 \
	} while (0)
#else
#define PRINT_INFO(fmt, args...)
#endif

#else

#if PRINT_DEBUG_ENABLE
#define PRINT_DEBUG(fmt, args...)                         \
	do                                                    \
	{                                                     \
		(printf("\r\n[DEBUG] >> "), printf(fmt, ##args)); \
	} while (0)
#else
#define PRINT_DEBUG(fmt, args...)
#endif

#if PRINT_ERR_ENABLE
#define PRINT_ERR(fmt, args...)                         \
	do                                                  \
	{                                                   \
		(printf("\r\n[ERR] >> "), printf(fmt, ##args)); \
	} while (0)
#else
#define PRINT_ERR(fmt, args...)
#endif

#if PRINT_INFO_ENABLE
#define PRINT_INFO(fmt, args...)                         \
	do                                                   \
	{                                                    \
		(printf("\r\n[INFO] >> "), printf(fmt, ##args)); \
	} while (0)
#else
#define PRINT_INFO(fmt, args...)
#endif

#endif

/**@} */

// 针对不同的编译器调用不同的stdint.h文件
#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
#include <stdint.h>
#endif

/* 断言 Assert */
#define AssertCalled(char, int) printf("\nError:%s,%d\r\n", char, int)
#define ASSERT(x) \
	if ((x) == 0) \
	AssertCalled(__FILE__, __LINE__)

typedef enum
{
	ASSERT_ERR = 0,				 /* 错误 */
	ASSERT_SUCCESS = !ASSERT_ERR /* 正确 */
} Assert_ErrorStatus;

#ifdef bool
#error "bool is already defined! Source: 这个文件之前包含了定义bool的头文件"
#endif

typedef enum
{
	FALSE = 0,	   /* 假 */
	TRUE = !FALSE, /* 真 */
} bool;

void DEBUG_USART_Config(void);

#ifdef __cplusplus
}
#endif

#endif
