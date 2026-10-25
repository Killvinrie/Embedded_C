```c
void vListInsert( List_t * const pxList, ListItem_t * const pxNewListItem )

{

	ListItem_t * pxIterator;

	const TickType_t xValueOfInsertion = pxNewListItem->xItemValue;

	if( xValueOfInsertion == portMAX_DELAY )

	{
		pxIterator = pxList->xListEnd.pxPrevious;
	}

	else
	{
		for( pxIterator = ( ListItem_t * ) &( pxList->xListEnd ); pxIterator->pxNext->xItemValue <= xValueOfInsertion; pxIterator = pxIterator->pxNext )

	{

/* There is nothing to do here, just iterating to the wanted  insertion position.

* IF YOU FIND YOUR CODE STUCK HERE, SEE THE NOTE JUST ABOVE.

*/
	}

	}

	pxNewListItem->pxNext = pxIterator->pxNext;

	pxNewListItem->pxNext->pxPrevious = pxNewListItem;

	pxNewListItem->pxPrevious = pxIterator;

	pxIterator->pxNext = pxNewListItem;


/* Remember which list the item is in. This allows fast removal of the

* item later. */

	pxNewListItem->pxContainer = pxList;

  

	( pxList->uxNumberOfItems ) = ( UBaseType_t ) ( pxList->uxNumberOfItems + 1U );

  



}
```



==🔴其实，链表插入就是完成两件事情。==

**第一步：让新节点找到自己的邻居。**

```c
pxNewListItem->pxPrevious = pxIterator;
pxNewListItem->pxNext = pxIterator->pxNext;
```

此时：

```
原链表：

A ⇄ B

新节点：

C.prev → A
C.next → B
```

**第二步：让原来的邻居认识新节点。**

```c
pxIterator->pxNext->pxPrevious = pxNewListItem;
pxIterator->pxNext = pxNewListItem;
```

最终：

```
A ⇄ C ⇄ B
```

注意，这种写法与 FreeRTOS 原始写法不同，但在前述条件下同样正确。