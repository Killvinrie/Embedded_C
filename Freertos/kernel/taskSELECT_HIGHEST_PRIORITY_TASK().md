``#define taskSELECT_HIGHEST_PRIORITY_TASK() 

```c
do { \

UBaseType_t uxTopPriority = uxTopReadyPriority; \

\

/* Find the highest priority queue that contains ready tasks. */ \

while( listLIST_IS_EMPTY( &( pxReadyTasksLists[ uxTopPriority ] ) ) != pdFALSE ) \

{ \

--uxTopPriority; \

} \

\

/* listGET_OWNER_OF_NEXT_ENTRY indexes through the list, so the tasks of \

* the same priority get an equal share of the processor time. */ \

listGET_OWNER_OF_NEXT_ENTRY( pxCurrentTCB, &( pxReadyTasksLists[ uxTopPriority ] ) ); \

uxTopReadyPriority = uxTopPriority; \

} while( 0 ) /* taskSELECT_HIGHEST_PRIORITY_TASK */

```

# 调度器在while循环中按照优先级降低的方向不断查找优先级列表是否存在任务，到优先级0怎么办？

**Freertos 始终创建空闲任务（Idle Task），其优先级为 0，因此正常运行的调度器总有可调度的任务。**


[[listGET_OWNER_OF_NEXT_ENTRY()]]函数将[[pxCurrentTCB]]指向即将要运行task所属的TCB