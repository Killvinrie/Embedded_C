```c
typedef struct QueueDefinition
{
    int8_t *pcHead;
    int8_t *pcWriteTo;

    union
    {
        QueuePointers_t xQueue;
        // ...
    } u;

    List_t xTasksWaitingToSend;
    List_t xTasksWaitingToReceive;

    volatile UBaseType_t uxMessagesWaiting;

    UBaseType_t uxLength;
    UBaseType_t uxItemSize;

    // ...
} Queue_t;
```
|成员|含义|
|---|---|
|`pcHead`|数据存储区起始地址|
|`pcWriteTo`|下一次写入位置|
|`u.xQueue.pcTail`|数据存储区末尾之后的地址|
|`u.xQueue.pcReadFrom`|上一次读取位置|
|`uxMessagesWaiting`|当前消息数量|
|`uxLength`|最大消息数量|
|`uxItemSize`|单条消息大小|
```c
typedef struct QueuePointers
{
    int8_t *pcTail;
    int8_t *pcReadFrom;
} QueuePointers_t;
```