# 中断向量表与 VTOR

## 向量表是什么？

保存 **初始 MSP**、**Reset_Handler**、其他异常和外设 ISR 的入口地址。每项通常占 4 字节。

外设中断号 `IRQn ≥ 0` 时：

```text
向量表项地址 = VTOR + 4 × (16 + IRQn)
```

CPU 取出该表项保存的 ISR 入口，然后跳转执行。

## 内容变化 vs 位置变化

- **改内容**：替换某个 ISR 入口地址；Flash 向量表通常在固件编译/更新时变化，RAM 向量表可以按设计动态修改。
- **改位置**：修改 `SCB->VTOR`，指定 CPU 使用哪张表；这不会改变表内数据，也不会自动跳转 App。

| 当前程序 | VTOR 示例 |
|---|---|
| Bootloader | `0x08000000` |
| Bank A | `0x08010000` |
| Bank B | `0x08040000` |

## 为什么 A/B 各有向量表？

同样的源代码链接在不同 Flash 地址，`Reset_Handler`、`USART1_IRQHandler` 等函数的绝对地址会不同。Bank B 如仍使用 A 的向量表，中断就可能错误地跳入 A 的 ISR。

```c
SCB->VTOR = 0x08040000U; // 使用 Bank B 的向量表
__DSB();
__ISB();
```

## 什么操作不修改向量表？

- 普通 UART、ADC、TIM 中断发生：查表执行 ISR。
- `HAL_NVIC_EnableIRQ()`：修改 NVIC 中断使能，不改向量表。
- 修改中断优先级、FreeRTOS PendSV 任务切换：通常不改 VTOR。

## OTA 与向量表

写入新的 `app_B.bin` 时，Bank B 的向量表内容也作为固件的一部分被写入；只有准备执行 Bank B 时才设置 VTOR 指向 B。

关联：[[Reset_Handler与程序跳转]]、[[10-汽车逆变器中的向量表]]。
