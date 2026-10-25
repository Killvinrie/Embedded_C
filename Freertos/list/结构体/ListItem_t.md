```c
struct xLIST_ITEM

{

listFIRST_LIST_ITEM_INTEGRITY_CHECK_VALUE /**< Set to a known value if configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES is set to 1. */

configLIST_VOLATILE TickType_t xItemValue; /**< The value being listed. In most cases this is used to sort the list in ascending order. */

struct xLIST_ITEM * configLIST_VOLATILE pxNext; /**< Pointer to the next ListItem_t in the list. */

struct xLIST_ITEM * configLIST_VOLATILE pxPrevious; /**< Pointer to the previous ListItem_t in the list. */

void * pvOwner; /**< Pointer to the object (normally a TCB) that contains the list item. There is therefore a two way link between the object containing the list item and the list item itself. */

struct xLIST * configLIST_VOLATILE pxContainer; /**< Pointer to the list in which this list item is placed (if any). */

listSECOND_LIST_ITEM_INTEGRITY_CHECK_VALUE /**< Set to a known value if configUSE_LIST_DATA_INTEGRITY_CHECK_BYTES is set to 1. */

};

typedef struct xLIST_ITEM ListItem_t;
```

`
`xItemValue`是delay列表中解除任务delay的时间点，由[[xTickCount]]与[[vTaskDelay()]]传入的timetodelay参数求和得到