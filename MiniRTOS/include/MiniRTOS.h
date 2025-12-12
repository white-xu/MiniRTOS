#ifndef MINIRTOS_H
#define MINIRTOS_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "projdefs.h"
#include "protable.h"

#define vAssertCalled(char,int) printf("Error:%s,%d\r\n",char,int)
#define configASSERT(x) if((x)==0) vAssertCalled(__FILE__,__LINE__)

#define xPortPendSVHandler 	PendSV_Handler
#define vPortSVCHandler 	SVC_Handler

#ifdef __cplusplus
}
#endif

#endif
