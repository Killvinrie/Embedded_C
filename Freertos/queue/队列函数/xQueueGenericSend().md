```c
BaseType_t xQueueGenericSend(
    QueueHandle_t xQueue,
    const void * const pvItemToQueue,
    TickType_t xTicksToWait,
    const BaseType_t xCopyPosition
);
```
|参数|含义|
|---|---|
|`xQueue`|队列句柄，本质上是指向 `Queue_t` 的指针|
|`pvItemToQueue`|待发送数据的地址|
|`xTicksToWait`|队列满时允许等待的 Tick 数|
|`xCopyPosition`|指定向队列头部、尾部发送或覆盖|


```c
for (;;)
{
    taskENTER_CRITICAL();

    if ((pxQueue->uxMessagesWaiting < pxQueue->uxLength) ||
        (xCopyPosition == queueOVERWRITE))
    {
        // 队列有空间，执行发送
    }
    else
    {
        // 队列已满
    }

    taskEXIT_CRITICAL();

    // 队列满时可能继续处理阻塞等待
}
```
判断队列是否已满，未满的话执行发送函数[[prvCopyDataToQueue]]将数据拷贝到队列中


拷贝完成后尝试唤醒接收任务
```c
if (listLIST_IS_EMPTY(
        &(pxQueue->xTasksWaitingToReceive)) == pdFALSE)
{
    if (xTaskRemoveFromEventList(
            &(pxQueue->xTasksWaitingToReceive)) != pdFALSE)
    {
        queueYIELD_IF_USING_PREEMPTION();
    }
}
```




