```c
void xPortSysTickHandler( void )

{

/* The SysTick runs at the lowest interrupt priority, so when this interrupt

* executes all interrupts must be unmasked. There is therefore no need to

* save and then restore the interrupt mask value as its value is already

* known. */

	portDISABLE_INTERRUPTS();

	traceISR_ENTER();



/* Increment the RTOS tick. */

	if( xTaskIncrementTick() != pdFALSE )

	{

		traceISR_EXIT_TO_SCHEDULER();

  

/* A context switch is required. Context switching is performed in

* the PendSV interrupt. Pend the PendSV interrupt. */

		portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;

	}

	else

	{

		traceISR_EXIT();

	}



	portENABLE_INTERRUPTS();

}
```
`portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;`
启动[[xPortPendSVHandler( void )]]调度

在每一个systick周期都会在[[xTaskIncrementTick()]]中主要实现：
- 检查delay列表中是否存在ListItem小于[[xTickCount]]的列表项
- 将所有满足时间的列表项从delay列表删除，并且根据优先级分别加入ready列表