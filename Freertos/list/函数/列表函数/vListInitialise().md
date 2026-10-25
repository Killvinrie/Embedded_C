```c
void vListInitialise(List_t * const pxList)
{
    pxList->pxIndex =
        (ListItem_t *)&pxList->xListEnd;

    pxList->xListEnd.xItemValue = portMAX_DELAY;

    pxList->xListEnd.pxNext =
        (ListItem_t *)&pxList->xListEnd;

    pxList->xListEnd.pxPrevious =
        (ListItem_t *)&pxList->xListEnd;

    pxList->uxNumberOfItems = 0;
}

```