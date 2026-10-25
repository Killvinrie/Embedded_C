```c
void vListInsertEnd( List_t * const pxList, ListItem_t * const pxNewListItem )

{

ListItem_t * const pxIndex = pxList->pxIndex;



/* Insert a new list item into pxList, but rather than sort the list,

* makes the new list item the last item to be removed by a call to

* listGET_OWNER_OF_NEXT_ENTRY(). */

pxNewListItem->pxNext = pxIndex;

pxNewListItem->pxPrevious = pxIndex->pxPrevious;

 
/* Only used during decision coverage testing. */

mtCOVERAGE_TEST_DELAY();

  
pxIndex->pxPrevious->pxNext = pxNewListItem;

pxIndex->pxPrevious = pxNewListItem;


/* Remember which list the item is in. */

pxNewListItem->pxContainer = pxList;


( pxList->uxNumberOfItems ) = ( UBaseType_t ) ( pxList->uxNumberOfItems + 1U );

}


```

# [[question]]使用场景？




