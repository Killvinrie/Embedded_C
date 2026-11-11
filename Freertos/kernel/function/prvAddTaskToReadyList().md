在delay时间结束后或者任务创建时将任务加入readylist
```c
define prvAddTaskToReadyList( pxTCB ) \

do { \

traceMOVED_TASK_TO_READY_STATE( pxTCB ); \

taskRECORD_READY_PRIORITY( ( pxTCB )->uxPriority ); \

listINSERT_END( &( pxReadyTasksLists[ ( pxTCB )->uxPriority ] ), &( ( pxTCB )->xStateListItem ) ); \

tracePOST_MOVED_TASK_TO_READY_STATE( pxTCB ); \

} while( 0 )
```

`taskRECORD_READY_PRIORITY( ( pxTCB )->uxPriority ); `记录此任务的优先级,与[[uxTopReadyPriority]]做对比,若优先级大于[[uxTopReadyPriority]],将[[uxTopReadyPriority]]更新