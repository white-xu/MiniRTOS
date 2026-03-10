# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

MiniRTOS is a minimal real-time operating system kernel based on FreeRTOS, designed for learning RTOS internals. It implements core RTOS features: task scheduling and queue-based inter-task communication. The project runs on STM32F407 hardware and provides a simplified codebase for understanding RTOS design principles.

## Build System

This project uses Keil MDK (µVision) as the build system:
- Project file: `Project/MiniRTOS.uvprojx`
- Build the project using Keil µVision IDE
- Target hardware: STM32F407 (ARM Cortex-M4)
- No Makefile or command-line build system is provided

## Architecture

### Core Components

**MiniRTOS Kernel** (`MiniRTOS/`)
- `task.c/h` - Task management: creation, scheduling, delays, blocking
- `queue.c/h` - Queue implementation for inter-task communication
- `list.c/h` - Doubly-linked list data structure used by scheduler
- `protable/RVDS/port.c` - ARM Cortex-M4 port-specific code (context switching)
- `protable/RVDS/portmacro.h` - Port-specific macros and type definitions
- `protable/MemMang/heap.c` - Memory management (heap allocation)

**Hardware Abstraction**
- `Libraries/CMSIS/` - ARM CMSIS headers for Cortex-M4
- `Libraries/STM32F4xx_HAL_Driver/` - STM32 HAL drivers
- `SYSTEM/` - System utilities (Debug, Delay)
- `User/` - Application code and HAL configuration

**Debugging**
- `cm_backtrace/` - Stack backtrace library for fault analysis

### Key Design Patterns

**Task Scheduling**
- Priority-based preemptive scheduler
- Ready tasks organized in priority lists (`pxReadyTasksLists[]`)
- Delayed tasks managed with two lists to handle tick overflow
- Context switching via PendSV exception (lowest priority)

**Task States**
- Tasks tracked via Task Control Block (TCB) containing stack pointer, state list item, event list item, priority, and name
- State transitions managed through list operations

**Queue Mechanism**
- Blocking send/receive with timeout support
- Tasks block on event lists when queue is full/empty
- Automatic task unblocking when queue state changes

## API Usage

**Task Management**
```c
BaseType_t xTaskCreate(TaskFunction_t pxTaskCode,
                       const char *const pcName,
                       const uint16_t usStackDepth,
                       void *const pvParameters,
                       UBaseType_t uxPriority,
                       TaskHandle_t *const pxCreatedTask);
void vTaskStartScheduler(void);
void vTaskDelay(const TickType_t xTicksToDelay);
```

**Queue Operations**
```c
QueueHandle_t xQueueCreate(const UBaseType_t uxQueueLength, const UBaseType_t uxItemSize);
BaseType_t xQueueSend(QueueHandle_t xQueue, const void *const pvItemToQueue, TickType_t xTicksToWait);
BaseType_t xQueueReceive(QueueHandle_t xQueue, void *const pvBuffer, TickType_t xTicksToWait);
```

**Return Values**
- `pdPASS` - Operation successful
- `pdFAIL` - Operation failed
- `portMAX_DELAY` - Block indefinitely

## Critical Sections

The kernel uses critical sections to protect shared data:
- `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()` - Disable/enable interrupts
- Implemented via BASEPRI register manipulation on Cortex-M4
- Used around scheduler data structure modifications

## Interrupt Handling

- SysTick timer drives the scheduler tick (`xTaskIncrementTick()`)
- PendSV exception performs context switching
- SVC exception starts the first task
- Interrupt handlers defined in `User/stm32f4xx_it.c`

## Configuration

Key configuration in `MiniRTOS/include/MiniRTOS.h`:
- `configMINIMAL_STACK_SIZE` - Minimum task stack size (128 words)
- `configASSERT(x)` - Assertion macro for debugging

## Code Conventions

- Chinese comments are used throughout the codebase
- FreeRTOS naming conventions: `x` prefix for variables, `v` for void functions, `px` for pointers
- Type suffixes: `_t` for types, `Handle_t` for opaque handles
