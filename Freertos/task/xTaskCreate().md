

```c
BaseType_t xTaskCreate( TaskFunction_t pxTaskCode,

	const char * const pcName,

	const configSTACK_DEPTH_TYPE uxStackDepth,

	void * const pvParameters,

	UBaseType_t uxPriority,

	TaskHandle_t * const pxCreatedTask )

{

TCB_t * pxNewTCB;

BaseType_t xReturn;

  
pxNewTCB = prvCreateTask( pxTaskCode, pcName, uxStackDepth, pvParameters, uxPriority, pxCreatedTask );
 

if( pxNewTCB != NULL )

{

	prvAddNewTaskToReadyList( pxNewTCB );

	xReturn = pdPASS;

}

else

{

	xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;

}
 

return xReturn;

}
```

| 参数              | 含义          |
| --------------- | ----------- |
| `pvTaskCode`    | 任务入口函数      |
| `pcName`        | 任务名称        |
| `usStackDepth`  | 任务栈深度       |
| `pvParameters`  | 传入任务的参数     |
| `uxPriority`    | 任务优先级       |
| `pxCreatedTask` | 用于接收任务句柄的指针 |
