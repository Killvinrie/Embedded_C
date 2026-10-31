```c
STATIC void prvAddCurrentTaskToDelayedList( TickType_t xTicksToWait,

const BaseType_t xCanBlockIndefinitely )

{

TickType_t xTimeToWake;

const TickType_t xConstTickCount = xTickCount;

List_t * const pxDelayedList = pxDelayedTaskList;

List_t * const pxOverflowDelayedList = pxOverflowDelayedTaskList;
```


# 被什么函数调用?
[[vTaskPlaceOnEventList()]]     
[[vTaskDelay()]]


# 函数做了什么?
检查当前时间加上xTicksToWait后的时间是否异常(溢出)
- 不异常的话将当前任务链表项依据itemvalue大小插入delaylist
- 异常的话将当前任务依据itemvalue大小插入overflowdelaylist