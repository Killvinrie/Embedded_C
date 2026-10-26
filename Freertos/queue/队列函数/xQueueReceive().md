```c
BaseType_t xQueueReceive(
    QueueHandle_t xQueue,
    void * const pvBuffer,
    TickType_t xTicksToWait
);
```

```c
BaseType_t xQueueReceive(
    QueueHandle_t xQueue,
    void * const pvBuffer,
    TickType_t xTicksToWait
)
{
    Queue_t * const pxQueue = xQueue;

    for (;;)
    {
        taskENTER_CRITICAL();

        if (pxQueue->uxMessagesWaiting > 0)
        {
            // ① 从队列复制数据
            prvCopyDataFromQueue(pxQueue, pvBuffer);

            // ② 消息数量减一
            pxQueue->uxMessagesWaiting--;

            // ③ 检查有没有等待发送的任务
            if (listLIST_IS_EMPTY(
                &pxQueue->xTasksWaitingToSend) == pdFALSE)
            {
                if (xTaskRemoveFromEventList(
                    &pxQueue->xTasksWaitingToSend) != pdFALSE)
                {
                    queueYIELD_IF_USING_PREEMPTION();
                }
            }

            taskEXIT_CRITICAL();
            return pdPASS;
        }

        // 队列为空，且不允许等待
        if (xTicksToWait == 0)
        {
            taskEXIT_CRITICAL();
            return errQUEUE_EMPTY;
        }

        // 实际源码还会记录超时状态
        taskEXIT_CRITICAL();

        // 进入后续的超时检查和阻塞逻辑
    }
}
```

