```c
/*

* Definition of the type of queue used by the scheduler.

*/

typedef struct xLIST
{
listFIRST_LIST_INTEGRITY_CHECK_VALUE /**< Set to a known value if configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES is set to 1. */

configLIST_VOLATILE UBaseType_t uxNumberOfItems;

ListItem_t * configLIST_VOLATILE pxIndex; /**< Used to walk through the list. Points to the last item returned by a call to listGET_OWNER_OF_NEXT_ENTRY (). */

MiniListItem_t xListEnd; /**< List item that contains the maximum possible item value meaning it is always at the end of the list and is therefore used as a marker. */

listSECOND_LIST_INTEGRITY_CHECK_VALUE /**< Set to a known value if configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES is set to 1. */

} List_t;
```



`pxIndex` 不是永远指向第一个节点，而是一个可以移动的遍历指针。

FreeRTOS 使用这个指针实现链表的循环遍历，其中一个重要用途就是同优先级任务之间的轮转调度。

