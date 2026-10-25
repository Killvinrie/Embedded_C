```c
void vTaskDelay( const TickType_t xTicksToDelay )

{

BaseType_t xAlreadyYielded = pdFALSE;

  

traceENTER_vTaskDelay( xTicksToDelay );

  

/* A delay time of zero just forces a reschedule. */

if( xTicksToDelay > ( TickType_t ) 0U )

{

vTaskSuspendAll();

{

configASSERT( uxSchedulerSuspended == 1U );

  

traceTASK_DELAY();

  

/* A task that is removed from the event list while the

* scheduler is suspended will not get placed in the ready

* list or removed from the blocked list until the scheduler

* is resumed.

*

* This task cannot be in an event list as it is the currently

* executing task. */

prvAddCurrentTaskToDelayedList( xTicksToDelay, pdFALSE );

}

xAlreadyYielded = xTaskResumeAll();

}

else

{

mtCOVERAGE_TEST_MARKER();

}

  

/* Force a reschedule if xTaskResumeAll has not already done so, we may

* have put ourselves to sleep. */

if( xAlreadyYielded == pdFALSE )

{

taskYIELD_WITHIN_API();

}

else

{

mtCOVERAGE_TEST_MARKER();

}

  

traceRETURN_vTaskDelay();

}
```

核心：`prvAddCurrentTaskToDelayedList(xTicksToDelay, pdFALSE)` 
==🔴将当前task从ready_list迁移到delay_list中

