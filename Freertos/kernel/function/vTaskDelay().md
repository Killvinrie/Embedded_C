```c
void vTaskDelay( const TickType_t xTicksToDelay )

{

/* A delay time of zero just forces a reschedule. */

if( xTicksToDelay > ( TickType_t ) 0U )

{
	prvAddCurrentTaskToDelayedList( xTicksToDelay, pdFALSE );
}

}
```

核心：[[prvAddCurrentTaskToDelayedList()]]
==🔴将当前task从ready_list迁移到delay_list中

