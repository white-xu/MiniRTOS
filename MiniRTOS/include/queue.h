#ifndef INC_QUEUE_H
#define INC_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void * QueueHandle_t;

QueueHandle_t xQueueCreate(const UBaseType_t uxQueueLength, const UBaseType_t uxItemSize);
BaseType_t xQueueSend(QueueHandle_t xQueue, const void *const pvItemToQueue, TickType_t xTicksToWait);
BaseType_t xQueueReceive(QueueHandle_t xQueue, void *const pvBuffer, TickType_t xTicksToWait);

#ifdef __cplusplus
}
#endif

#endif
