
# 从链表中删除节点

## 1. 核心源码
```c
UBaseType_t uxListRemove(
    ListItem_t * const pxItemToRemove
)
{
    List_t * const pxList =
        pxItemToRemove->pxContainer;

    pxItemToRemove->pxNext->pxPrevious =
        pxItemToRemove->pxPrevious;

    pxItemToRemove->pxPrevious->pxNext =
        pxItemToRemove->pxNext;

    if (pxList->pxIndex == pxItemToRemove)
    {
        pxList->pxIndex =
            pxItemToRemove->pxPrevious;
    }

    pxItemToRemove->pxContainer = NULL;

    pxList->uxNumberOfItems--;

    return pxList->uxNumberOfItems;
}
```

## 第一部分：解除前后节点的链接关系



## 第二部分：为什么需要修改 pxIndex？