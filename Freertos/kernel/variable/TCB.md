```c
typedef struct tskTaskControlBlock
{
    volatile StackType_t *pxTopOfStack;

    ListItem_t xStateListItem;

    ListItem_t xEventListItem;

    UBaseType_t uxPriority;

    StackType_t *pxStack;

    char pcTaskName[configMAX_TASK_NAME_LEN];

    // 其他成员...
} TCB_t;

```
TCB 可以理解为任务的**档案**。

它记录了任务的栈顶位置、优先级、任务状态链表节点等重要信息。

其中：

- `pxTopOfStack`：任务当前栈顶指针。
- `xStateListItem`：用于把任务连接到就绪、阻塞等状态链表。
- `xEventListItem`：用于把任务连接到事件等待链表等。
- `uxPriority`：任务优先级。
- `pxStack`：任务栈空间的起始地址。

注意，**TCB 只是记录任务信息的数据结构，并不是任务本身的执行代码。**