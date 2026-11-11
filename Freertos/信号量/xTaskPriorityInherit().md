```c
if( pxMutexHolderTCB->uxPriority < pxCurrentTCB->uxPriority )

{

	if( listIS_CONTAINED_WITHIN( &( pxReadyTasksLists[ pxMutexHolderTCB->uxPriority ] ), &( pxMutexHolderTCB->xStateListItem ) ) != pdFALSE )

	{
		pxMutexHolderTCB->uxPriority = pxCurrentTCB->uxPriority;
		prvAddTaskToReadyList( pxMutexHolderTCB );
	}
else
{
/* Just inherit the priority. */

	pxMutexHolderTCB->uxPriority = pxCurrentTCB->uxPriority;

}
}
```

如果发现该task已经在[[pxReadyTasksLists]]中,将task的优先级升级,并且将该task更新到对应更高优先级的列表