#ifndef PORTABLE_H
#define PORTABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "portmacro.h"

#define portBYTE_ALIGNMENT 8
#define portBYTE_ALIGNMENT_MASK ( 0x0007 )

StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters );
BaseType_t xPortStartScheduler( void );

void *pvPortMalloc( size_t xSize );
void vPortFree( void *pv );
size_t xPortGetFreeHeapSize( void );
size_t xPortGetMinimumEverFreeHeapSize( void );

#ifdef __cplusplus
}
#endif

#endif
