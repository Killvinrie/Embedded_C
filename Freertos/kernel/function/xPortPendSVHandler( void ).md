## 一、PendSV 完整代码

以下保留 FreeRTOS GCC/ARM_CM3 的核心指令及其执行顺序，为每条指令添加中文注释。

```
/* PendSV异常处理函数，不生成普通函数的入栈和出栈代码 */
void xPortPendSVHandler(void) __attribute__((naked));

void xPortPendSVHandler(void)
{
    __asm volatile
    (
        /* ====== 1. 保存当前任务上下文 ====== */

        "mrs r0, psp                 \n" // ① 将当前任务的PSP读取到R0
        "isb                         \n" // ② 指令同步屏障

        "ldr r3, =pxCurrentTCB       \n" // ③ R3 = &pxCurrentTCB
        "ldr r2, [r3]                \n" // ④ R2 = pxCurrentTCB，即当前任务TCB地址

        "stmdb r0!, {r4-r11}         \n" // ⑤ 将R4~R11压入当前任务栈，更新R0
        "str r0, [r2]                \n" // ⑥ 将新的栈顶地址保存到TCB的pxTopOfStack

        /* ====== 2. 调用任务调度器 ====== */

        "stmdb sp!, {r3, r14}        \n" // ⑦ 将R3和R14临时保存到MSP栈


        "bl vTaskSwitchContext       \n" // ⑩ 调用调度器，更新pxCurrentTCB

    
        "ldmia sp!, {r3, r14}        \n" // ⑬ 从MSP恢复R3和R14

        /* ====== 3. 恢复新任务上下文 ====== */

        "ldr r1, [r3]                \n" // ⑭ R1 = pxCurrentTCB，新任务TCB地址
        "ldr r0, [r1]                \n" // ⑮ R0 = 新任务TCB中的pxTopOfStack

        "ldmia r0!, {r4-r11}         \n" // ⑯ 从新任务栈恢复R4~R11，更新R0

        "msr psp, r0                 \n" // ⑰ 将新任务的栈顶地址写入PSP
        "isb                         \n" // ⑱ 指令同步屏障

        "bx r14                      \n" // ⑲ 根据EXC_RETURN执行异常返回

        :
        : "i"(configMAX_SYSCALL_INTERRUPT_PRIORITY)
    );
}
```

---



---

## 总结

PendSV 的执行流程：
- 首先利用r0保存taskA的栈顶，r2保存A的`pxTopOfStack`, r3保存[[pxCurrentTCB]]的地址
- 接着开始调度，将[[pxCurrentTCB]]更新为B的TCB  
- 通过TCB_B的`pxTopOfStack`以及栈顶工具寄存器r0恢复r4-r11
- 最后把恢复之后栈顶的位置r0给到栈指针psp
- 异常返回


**Cortex-M3 在进入 PendSV 异常时，硬件已经自动将 R0～R3、R12、LR、PC 和 xPSR 保存到任务栈中。**


#### `bx r14`

```asm
bx r14
```

`BX`：Branch and Exchange。

由于 R14 保存的是 EXC_RETURN，例如：

```text
0xFFFFFFFD
```

Cortex-M3 会将其识别为异常返回请求。

随后硬件自动从当前 PSP 指向的异常栈帧中恢复：

```text
R0
R1
R2
R3
R12
LR
PC
xPSR
```

由于 PSP 已指向 TaskB 栈，因此恢复的是 TaskB 的寄存器。

其中 PC 决定 TaskB 从哪里继续执行。

**至此，任务上下文切换完成。**
```

### 五个最重要的知识点

1. **PSP**：用于保存和恢复普通任务的上下文。
    
2. **MSP**：PendSV 异常处理程序自身使用的栈。
    
3. `**pxCurrentTCB**`：在 `vTaskSwitchContext()` 中更新为选中任务的 TCB。
    
4. `**pxTopOfStack**`：保存任务上下文对应的栈顶地址。
    
5. **EXC_RETURN**：通过 `BX R14` 触发硬件异常返回，从新任务的栈中恢复 PC 等寄存器。
    

**核心结论：PendSV 通过切换任务栈并恢复 CPU 寄存器，实现从旧任务到新任务的执行状态切换，而不需要复制整个任务栈。**