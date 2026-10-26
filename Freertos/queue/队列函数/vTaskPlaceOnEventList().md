```c
void vTaskPlaceOnEventList( List_t * const pxEventList,

const TickType_t xTicksToWait )

{
	vListInsert( pxEventList, &( pxCurrentTCB->xEventListItem ) );
	
	prvAddCurrentTaskToDelayedList( xTicksToWait, pdTRUE );

}

```


由[[xQueueSemaphoreTake()]]或者[[xQueueGenericSend()]]和[[xQueueReceive()]]函数调用

将当前任务同时插入队列的waitinglist以及delaylist



