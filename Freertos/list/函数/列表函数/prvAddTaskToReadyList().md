
`#define prvAddTaskToReadyList( pxTCB ) `

```c

do { \

traceMOVED_TASK_TO_READY_STATE( pxTCB ); \

taskRECORD_READY_PRIORITY( ( pxTCB )->uxPriority ); \

listINSERT_END( &( pxReadyTasksLists[ ( pxTCB )->uxPriority ] ), &( ( pxTCB )->xStateListItem ) ); \

tracePOST_MOVED_TASK_TO_READY_STATE( pxTCB ); \

} while( 0 )
```