```c
void vTaskSuspendAll( void )

{

	traceENTER_vTaskSuspendAll();

  
	uxSchedulerSuspended = ( UBaseType_t ) ( uxSchedulerSuspended + 1U );


	traceRETURN_vTaskSuspendAll();

}
```

另 [[uxSchedulerSuspended]]变量加1;