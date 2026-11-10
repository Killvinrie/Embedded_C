调用[[vTaskSuspendAll]]时,此变量加一

当[[vTaskSwitchContext()]]执行时,检查此变量

```c
if( uxSchedulerSuspended != ( UBaseType_t ) 0U )

{

/* The scheduler is currently suspended - do not allow a context

* switch. */

xYieldPendings[ 0 ] = pdTRUE;

}
```