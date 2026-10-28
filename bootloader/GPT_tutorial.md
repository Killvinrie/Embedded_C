# STM32F411 A/B OTA 完整开发教程

## 第一部分：从 Cortex-M4 启动原理到 Bootloader 的底层实现

我们从最底层开始，逐步实现你的四旋翼 OTA 系统。

最终目标是：

**STM32F411 内部保存两份飞控程序：Bank A 和 Bank B。升级时将新固件写入非活动 Bank，验证成功后切换运行；如果新固件启动失败，自动回滚到旧版本。**

整个系统最终应该实现：

```
                  STM32F411 上电
                        │
                        ▼
                 ┌─────────────┐
                 │ Bootloader  │
                 └──────┬──────┘
                        │
                 读取启动信息
                        │
               ┌────────┴────────┐
               │                 │
               ▼                 ▼
          正常启动            固件升级
               │                 │
               ▼                 ▼
          选择 Bank          接收新固件
               │                 │
        ┌──────┴──────┐          ▼
        │             │      写入备用 Bank
        ▼             ▼          │
     Bank A        Bank B        ▼
        │             │      校验并切换
        └──────┬──────┘          │
               │                 │
               ▼                 │
         飞控程序运行 ◀───────────┘
               │
               ▼
         新版本启动确认
               │
          ┌────┴────┐
          │         │
         成功      失败
          │         │
          ▼         ▼
       确认新版本   回滚旧版本
```

这套机制并不是简单的 Flash 写入，而是一个完整的**启动管理系统（Boot Management System）**。

我建议把开发分为六个阶段：

|阶段|学习内容|实现目标|
|---|---|---|
|1|Cortex-M4 启动机制|理解 Bootloader 如何执行|
|2|链接脚本与程序跳转|实现 Bootloader 启动 App A/B|
|3|Flash 底层驱动|实现两个应用分区的擦写|
|4|Boot Metadata|实现启动选择与状态持久化|
|5|OTA 状态机与串口协议|实现 ESP32 向备用 Bank 更新固件|
|6|试运行与回滚|实现升级失败自动恢复|

**这一部分先深入阶段 1 和阶段 2，并建立可以运行的 Bootloader 基础工程。**

后面几个阶段建立在它们之上。

---

# 一、Bootloader 本质上是什么？

首先理解一个问题：

> STM32F411 上电以后，为什么能够自动运行你写的 `main()`？

我们平时使用 STM32CubeIDE 开发，通常从 `main.c` 开始编写代码：

```
int main(void)
{
    HAL_Init();

    SystemClock_Config();

    while (1)
    {
        // 飞控程序
    }
}
```

但事实上：

**CPU 上电后执行的第一段程序并不是 `main()`。**

在执行 `main()` 之前，至少要经历：

```
STM32 上电
    │
    ▼
硬件复位
    │
    ▼
读取中断向量表
    │
    ▼
加载初始 MSP
    │
    ▼
获取 Reset_Handler 地址
    │
    ▼
执行 Reset_Handler
    │
    ▼
执行 SystemInit()
    │
    ▼
初始化 .data 和 .bss
    │
    ▼
调用 main()
```

这里的每一步都与我们要实现的 Bootloader 有关。

## 1.1 Cortex-M4 如何启动程序？

STM32F411 采用 ARM Cortex-M4 内核。

Cortex-M4 的中断向量表中，前两个 32 位数据非常特殊：

```
向量表起始地址
       │
       ▼
┌──────────────────────────┐
│ 0x00000000               │
│ 初始 MSP                 │
├──────────────────────────┤
│ 0x00000004               │
│ Reset_Handler 地址       │
├──────────────────────────┤
│ 0x00000008               │
│ NMI_Handler              │
├──────────────────────────┤
│ 0x0000000C               │
│ HardFault_Handler        │
├──────────────────────────┤
│ ...                      │
└──────────────────────────┘
```

这是 ARM Cortex-M4 的启动规则。ST 官方 PM0214 手册第 2.3.4 节专门解释了向量表结构。[STMicroelectronics](https://www.st.com/resource/en/programming_manual/pm0214-stm32-cortexm4-mcus-and-mpus-programming-manual-stmicroelectronics.pdf?utm_source=chatgpt.com)

注意：这里的 `0x00000000` 是启动时访问的地址。对于正常从用户 Flash 启动的 F411，该地址区域映射到用户 Flash，因此实际数据来自 `0x08000000`。

假设 Flash 前 8 字节如下：

```
地址              内容

0x08000000        0x20020000
0x08000004        0x08000189
```

CPU 进行的操作可以理解为：

```
MSP = *(uint32_t *)0x08000000;

PC = *(uint32_t *)0x08000004;
```

也就是：

**第一步：设置栈指针。**

```
MSP = 0x20020000
```

**第二步：获得复位处理函数的地址并开始执行。**

```
Reset_Handler = 0x08000189
```

这里 `0x08000189` 最低位是 1，因为 Cortex-M4 使用 Thumb 指令集，该位表示 Thumb 状态，并不是普通的地址偏移。

所以，CPU 并不需要知道：

- `main()` 在哪里。
- 你的程序使用什么语言。
- 有没有 FreeRTOS。
- 程序中有没有 MPU6050 驱动。

它只需要按照芯片启动模式，找到向量表并执行对应的复位入口。

这一点非常关键。

**Bootloader 能够启动另一个程序，正是利用了这个原理。**

---

# 二、Reset_Handler 是如何找到 main() 的？

打开你的 STM32 工程，通常可以看到这样的文件：

```
startup_stm32f411xe.s
```

这是 Cortex-M4 的启动汇编文件。

你可以在自己的 Ubuntu 工程中找到它。

文件中通常包含类似的代码：

```
.section .isr_vector,"a",%progbits

.word _estack
.word Reset_Handler
.word NMI_Handler
.word HardFault_Handler
```

解释一下。

### 2.1 `.isr_vector`

这段汇编告诉链接器：

这里保存的是中断向量表。

通常对应链接脚本中的：

```
.isr_vector :
{
    KEEP(*(.isr_vector))
} >FLASH
```

也就是将中断向量表放到 Flash 对应区域。

如果 Flash 起始地址是：

```
0x08000000
```

那么中断向量表就在：

```
0x08000000
```

如果以后将 Application 的链接地址改为：

```
0x08010000
```

那么向量表也应该位于：

```
0x08010000
```

这就是我们之后必须修改链接脚本的原因。

### 2.2 `_estack`

`_estack` 表示初始化时的栈顶地址。

在链接脚本中通常可以看到：

```
_estack = ORIGIN(RAM) + LENGTH(RAM);
```

对于 128KB SRAM：

```
RAM 起始地址 = 0x20000000

RAM 大小 = 0x20000
```

因此：

```
_estack = 0x20020000
```

这是栈的初始顶部位置。

你之前学习过 FreeRTOS 的任务栈，可以注意这里的区别：

- CPU 复位时，首先使用 MSP。
- FreeRTOS 在 Cortex-M4 上通常让任务使用 PSP。
- Bootloader 一般采用裸机程序，使用 MSP 就足够了。

### 2.3 Reset_Handler

启动文件中的复位入口通常类似：

```
Reset_Handler:

    bl SystemInit

    /* 初始化 .data */

    /* 清零 .bss */

    bl main
```

具体代码由启动文件和工具链决定，有些工程通过 C 运行库完成部分初始化工作。

但整体逻辑相同。

这意味着：

```
CPU 读取 Reset_Handler
          │
          ▼
    Reset_Handler
          │
          ▼
       main()
```

而 Bootloader 要跳转到 Application，本质上就是：

**让 CPU 使用 Application 的初始 MSP，并执行 Application 的 Reset_Handler。**

现在我们就可以开始设计 Bootloader 了。

---

# 三、A/B Bootloader 的 Flash 设计

这里继续以你的 **STM32F411CEU6，512KB Flash、128KB SRAM** 为例。

首先明确：

F411 的 Flash 只有一个物理主存储块，不能像某些支持双 Bank 的 MCU 那样，一边执行程序，一边无阻塞地擦写另一 Bank。

RM0383 明确指出，Flash 擦写期间发生的读取会导致总线等待。因此我们的 A/B 是**软件逻辑分区**，而不是硬件双 Bank。[STMicroelectronics](https://www.st.com/resource/en/reference_manual/dm00119316-stm32f411xce-advanced-armbased-32bit-mcus-stmicroelectronics.pdf?utm_source=chatgpt.com)

## 3.1 最终采用的内存布局

沿用前面确定的分区：

```
STM32F411CEU6 Flash

0x08000000
┌────────────────────────────┐
│ Sector 0     16KB          │
│                            │
│ Bootloader                 │
├────────────────────────────┤ 0x08004000
│ Sector 1     16KB          │
│                            │
│ Bootloader                 │
├────────────────────────────┤ 0x08008000
│ Sector 2     16KB          │
│                            │
│ Metadata 0                 │
├────────────────────────────┤ 0x0800C000
│ Sector 3     16KB          │
│                            │
│ Metadata 1                 │
├────────────────────────────┤ 0x08010000
│ Sector 4     64KB          │
│                            │
│                            │
│ Bank A                     │
├────────────────────────────┤ 0x08020000
│ Sector 5     128KB         │
│                            │
│ Bank A                     │
│                            │
├────────────────────────────┤ 0x08040000
│ Sector 6     128KB         │
│                            │
│ Bank B                     │
│                            │
├────────────────────────────┤ 0x08060000
│ Sector 7     128KB         │
│                            │
│ Bank B                     │
│                            │
└────────────────────────────┘ 0x08080000
```

分区地址：

|分区|起始地址|大小|
|---|---|---|
|Bootloader|`0x08000000`|32KB|
|Metadata 0|`0x08008000`|16KB|
|Metadata 1|`0x0800C000`|16KB|
|Bank A|`0x08010000`|192KB|
|Bank B|`0x08040000`|256KB|

虽然 Bank B 有 256KB，但我们暂时将两份应用固件都限制为 **192KB**。

必须补充一个细节：Bank B 的末尾 64KB 并不是独立的可擦除分区，它位于 Sector 7 内。

擦除 Bank B 时，必须按 Sector 6 和 Sector 7 的完整扇区擦除。

同理，擦除 Bank A 时，必须操作 Sector 4 和 Sector 5。

因此，**不能只根据应用程序逻辑大小来决定 Flash 的物理擦除范围**。

## 3.2 为什么 Bootloader 不应该放在 Bank A 里面？

因为 OTA 需要反复擦除应用程序区域。

如果 Bootloader 与 Application 使用相同的可擦除 Sector：

```
擦除 Application
        │
        ▼
同时擦除 Bootloader
        │
        ▼
下一次复位无法正常执行引导程序
```

整个系统就失去了自主恢复能力。

所以必须保证：

```
Bootloader：固定区域

Bank A：可更新区域

Bank B：可更新区域
```

对 Bootloader 来说，最重要的职责之一就是：

**无论升级过程发生什么错误，都不能让普通 OTA 数据包擦除或写入 Bootloader。**

后续还可以通过 Flash 写保护进一步降低误擦写的风险。

---

# 四、开始编写第一个 Bootloader 工程

现在正式进入代码实现。

我们的第一个目标很明确：

**让 F411 上电后首先执行 Bootloader，然后根据指定的地址，跳转到 Bank A 或 Bank B 中的程序。**

这个阶段先不写 Flash 驱动，也不涉及 ESP32。

为什么这样安排？

因为只有弄清楚 Bootloader 如何跳转到两个不同的应用程序，后面才能实现固件切换、升级和回滚。

## 4.1 创建工程目录

建议建立如下目录：

```
STM32F411_OTA/
│
├── Bootloader/
│   │
│   ├── Core/
│   │   ├── Inc/
│   │   │   ├── main.h
│   │   │   ├── boot_config.h
│   │   │   └── boot_jump.h
│   │   │
│   │   └── Src/
│   │       ├── main.c
│   │       └── boot_jump.c
│   │
│   ├── Drivers/
│   │
│   └── STM32F411CEUx_FLASH.ld
│
└── Application/
    │
    ├── Core/
    ├── Drivers/
    │
    ├── linker/
    │   ├── app_A.ld
    │   └── app_B.ld
    │
    └── Makefile
```

现在只关注两个文件：

```
boot_config.h
boot_jump.c
```

之后逐步添加：

```
boot_flash.c
boot_metadata.c
boot_protocol.c
boot_manager.c
```

## 4.2 创建 Bootloader 工程

使用 STM32CubeMX 或 STM32CubeIDE：

**第一步：选择芯片。**

```
STM32F411CEU6
```

**第二步：配置调试接口。**

```
SYS → Debug → Serial Wire
```

保留 SWD，方便通过 ST-Link 下载和调试。

**第三步：先不启用复杂外设。**

第一版只需要：

- RCC 默认时钟。
- HAL。
- SWD。

暂时不需要：

- FreeRTOS。
- DMA。
- MPU6050。
- PWM。
- ESP32 UART。

后面实现 OTA 数据接收时，再加入 UART。

## 4.3 修改 Bootloader 链接脚本

打开：

```
STM32F411CEUx_FLASH.ld
```

找到 `MEMORY`：

```
MEMORY
{
    RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 128K

    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 512K
}
```

改成：

```
MEMORY
{
    RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 128K

    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 32K
}
```

这意味着：

```
Bootloader 可使用的 Flash：

0x08000000 ～ 0x08007FFF
```

现在编译 Bootloader 时，链接器就会将它的：

- `.isr_vector`
- `.text`
- `.rodata`

等需要存放在 Flash 中的内容安排到这 32KB 区域内。

如果 Bootloader 的 Flash 占用超出 32KB，链接器通常会报告区域溢出。

需要强调：

**链接脚本限制的是程序布局，不是硬件写保护。** 即使 Bootloader 只链接到 32KB，它依然可能通过错误的 Flash 擦写代码破坏其他区域，所以后续还需要专门做地址合法性检查。

---

# 五、设计 Bootloader 的跳转模块

这是我们首先要真正写出来的代码。

## 5.1 创建 boot_config.h

```
#ifndef BOOT_CONFIG_H
#define BOOT_CONFIG_H

#include <stdint.h>

/* Bootloader */
#define BOOT_START_ADDR   0x08000000U
#define BOOT_SIZE         (32U * 1024U)

/* Metadata */
#define META0_ADDR        0x08008000U
#define META1_ADDR        0x0800C000U

/* Application Bank */
#define BANK_A_ADDR       0x08010000U
#define BANK_B_ADDR       0x08040000U

/* 两个应用程序统一限制为 192KB */
#define APP_MAX_SIZE      (192U * 1024U)

/* SRAM */
#define SRAM_START_ADDR   0x20000000U
#define SRAM_END_ADDR     0x20020000U

typedef enum
{
    APP_BANK_A = 0,
    APP_BANK_B = 1

} AppBank_t;

#endif
```

为什么需要这个文件？

因为 Bootloader 后续需要操作：

```
Bank A 的起始地址

Bank B 的起始地址

Metadata 地址

Flash 擦除边界
```

如果这些地址散落在不同源文件中，后续修改分区会非常麻烦。

因此，最好集中管理。

---

## 5.2 创建 boot_jump.h

```
#ifndef BOOT_JUMP_H
#define BOOT_JUMP_H

#include <stdint.h>
#include <stdbool.h>

bool Boot_CheckAppVector(uint32_t app_addr);

void Boot_JumpToApp(uint32_t app_addr);

#endif
```

这里提供两个接口：

```
Boot_CheckAppVector();
```

判断应用程序的中断向量表是否基本合理。

```
Boot_JumpToApp();
```

真正执行程序跳转。

注意：这个阶段的检查只针对向量表，**还不能证明固件完整或可信**。以后必须加入 CRC 或哈希、版本及固件有效状态验证。

---

## 5.3 先实现 Application 地址检查

打开 `boot_jump.c`：

```
#include "boot_jump.h"
#include "boot_config.h"

#include "stm32f4xx_hal.h"

static bool Boot_GetAppBase(uint32_t addr)
{
    return (addr == BANK_A_ADDR) ||
           (addr == BANK_B_ADDR);
}

bool Boot_CheckAppVector(uint32_t app_addr)
{
    uint32_t app_msp;
    uint32_t app_reset;
    uint32_t reset_addr;

    /* 只允许启动 Bank A 或 Bank B */
    if (!Boot_GetAppBase(app_addr))
    {
        return false;
    }

    /* 读取应用程序向量表前两个字 */
    app_msp =
        *(volatile const uint32_t *)(app_addr);

    app_reset =
        *(volatile const uint32_t *)(app_addr + 4U);

    /*
     * 检查 MSP：
     * 1. 是否位于 SRAM 范围
     * 2. 是否满足 8 字节对齐
     */
    if ((app_msp <= SRAM_START_ADDR) ||
        (app_msp > SRAM_END_ADDR) ||
        ((app_msp & 0x7U) != 0U))
    {
        return false;
    }

    /*
     * Cortex-M4 使用 Thumb 指令集。
     * Reset_Handler 向量最低位应为 1。
     */
    if ((app_reset & 0x1U) == 0U)
    {
        return false;
    }

    /* 清除 Thumb 状态位，得到实际代码地址 */
    reset_addr = app_reset & ~0x1U;

    /* 判断 Reset_Handler 是否处于对应 Bank */
    if ((reset_addr < app_addr) ||
        (reset_addr >= (app_addr + APP_MAX_SIZE)))
    {
        return false;
    }

    return true;
}
```

我们逐行理解其中的关键代码。

### 第一处：读取 MSP

```
app_msp =
    *(volatile const uint32_t *)app_addr;
```

假设：

```
app_addr = 0x08010000
```

那么：

```
app_msp = *(uint32_t *)0x08010000;
```

就是读取 Bank A 中断向量表的第一个字。

这个值是 Application 的初始栈指针。

例如：

```
0x20020000
```

接下来检查这个地址是否处于 F411 的 SRAM 范围。

为什么要检查？

因为如果 Bank A 是空的，Flash 通常会读到：

```
0xFFFFFFFF
```

此时若直接把 MSP 设置为这个值，再执行程序，就会发生严重错误。

### 第二处：读取 Reset_Handler

```
app_reset =
    *(volatile const uint32_t *)(app_addr + 4U);
```

这是 Application 的复位入口。

例如：

```
0x080104DD
```

说明代码入口位于 Bank A 中。

如果地址不在当前 Bank 的范围内，就不能跳转。

### 第三处：为什么检查最低位？

```
if ((app_reset & 0x1U) == 0U)
```

Cortex-M4 只执行 Thumb 指令。

复位向量中的入口地址最低位需要为 1，表示 Thumb 状态。

因此，不能将一个偶数值直接当作正常的复位入口使用。

这也与你之前学习 FreeRTOS 时遇到的 Thumb 函数地址、任务初始栈构造等知识相关。

到这里，你就已经写出了 Bootloader 最基础的应用程序检查函数。

---

# 六、实现真正的程序跳转

现在我们有了：

```
Boot_CheckAppVector();
```

下一步要实现：

```
Boot_JumpToApp();
```

但这一步不能直接写成：

```
void (*App)(void) = (void *)0x08010000;

App();
```

原因有两个：

第一，`0x08010000` 保存的是初始 MSP，并不是函数入口地址。

第二，Bootloader 和 Application 有各自的运行环境。

直接执行 Application 的入口而不处理栈、中断和外设状态，很容易导致 HardFault。

因此，程序跳转需要几个步骤。

## 6.1 第一步：停止 Bootloader 使用的外设

在完整的 OTA 系统中，Bootloader 可能已经初始化：

```
UART

SysTick

GPIO

DMA

CRC
```

跳转前应完成相应的反初始化。

因为进入 Application 之后，Application 会按照自己的初始化程序重新配置外设。

比如：

```
HAL_UART_DeInit(&huart1);
```

但这里有个重要原则：

**不能在已经切换到 Application 栈以后，再调用 Bootloader 的 HAL 函数进行清理。**

所以整个顺序应该是：

```
外设反初始化
       │
       ▼
停止 SysTick
       │
       ▼
清理中断状态
       │
       ▼
重定位 VTOR
       │
       ▼
设置 MSP
       │
       ▼
跳转 Reset_Handler
```

对于我们的第一个实验，Bootloader 暂时没有初始化 UART、DMA 等外设，因此可以先省略这些外设清理操作。

ST 关于程序跳转的技术说明也特别强调了清理 SysTick、NVIC 和外设状态的重要性。[STMicroelectronics Community](https://community.st.com/stm32-mcus-60/how-to-jump-to-system-bootloader-from-application-code-on-stm32-microcontrollers-71?utm_source=chatgpt.com)

## 6.2 第二步：停止 SysTick

```
SysTick->CTRL = 0U;

SysTick->LOAD = 0U;

SysTick->VAL = 0U;
```

为什么要停止 SysTick？

因为 Bootloader 执行过：

```
HAL_Init();
```

HAL 通常会初始化 SysTick，周期性地产生系统节拍。

如果跳转过程中 SysTick 突然发生异常，而向量表或栈已经部分切换，就可能造成异常处理错误。

因此，先关闭它。

## 6.3 第三步：关闭并清理 NVIC 中断

可以先通过：

```
__disable_irq();
```

临时屏蔽可屏蔽中断。

随后清理 NVIC 状态。

对于 F411，可以使用：

```
for (uint32_t i = 0; i < 8U; i++)
{
    NVIC->ICER[i] = 0xFFFFFFFFU;

    NVIC->ICPR[i] = 0xFFFFFFFFU;
}
```

这里的：

```
NVIC->ICER
```

是中断使能清除寄存器。

而：

```
NVIC->ICPR
```

是中断挂起状态清除寄存器。

但注意，`__disable_irq()` 设置了 CPU 的 PRIMASK。如果跳转后始终不恢复中断，Application 中的 SysTick、UART 等中断就无法正常运行。

所以最终跳转代码中必须明确处理这个问题。

## 6.4 第四步：修改 VTOR

这一步尤其重要。

`VTOR` 全称：

**Vector Table Offset Register**

也就是中断向量表偏移寄存器。

如果启动 Bank A：

```
SCB->VTOR = BANK_A_ADDR;
```

如果启动 Bank B：

```
SCB->VTOR = BANK_B_ADDR;
```

比如：

```
Bank A：

VTOR = 0x08010000
```

那么 Application 的中断处理函数就从 Bank A 的向量表查找。

如果：

```
Bank B：

VTOR = 0x08040000
```

中断处理函数就从 Bank B 的向量表查找。

这点非常重要，因为你的四旋翼会使用：

- SysTick。
- UART 中断。
- SPI/I2C 中断。
- 定时器中断。
- 以后可能还会使用 FreeRTOS 的 PendSV。

如果 VTOR 仍然指向 Bootloader，中断发生时就可能进入错误的处理函数。

## 6.5 第五步：设置 MSP 并跳转

这是整个 Bootloader 的核心。

我们先看最关键的汇编指令：

```
msr msp, r0
```

它的作用是：

**将寄存器 R0 中的值写入 MSP。**

然后：

```
bx r1
```

将程序执行流程转移到寄存器 R1 指向的地址。

根据 ARM 的调用约定，函数前两个整数参数通常分别通过 R0、R1 传递。

我们可以借此实现一个底层跳转函数。

下面是适用于 GCC、以裸机特权线程模式运行的 Bootloader 示例：

```
__attribute__((naked, noreturn))
static void Boot_BranchToApp(
    uint32_t app_msp,
    uint32_t app_reset)
{
    __asm volatile
    (
        "msr msp, r0 \n"
        "cpsie i      \n"
        "bx r1        \n"
    );
}
```

解释：

```
msr msp, r0
```

将 Application 的初始栈指针写入 MSP。

```
cpsie i
```

恢复可屏蔽中断。

```
bx r1
```

跳转到 Application 的 Reset_Handler。

这里使用 `naked` 的原因是：

**切换 MSP 后，不希望编译器再生成依赖旧函数栈帧的普通函数返回代码。**

这个实现要求在执行最后一段汇编之前，已经清理好外设、中断和挂起状态，并设置好 VTOR。

它是裸机 Bootloader 的跳转入口示例，不应直接拿去从 FreeRTOS 任务或中断服务函数中调用。

## 6.6 组合成 Boot_JumpToApp()

下面把整个流程组合起来。

```
#include "boot_jump.h"
#include "boot_config.h"

#include "stm32f4xx_hal.h"

/*
 * Boot_CheckAppVector()
 * 使用前面实现的检查函数。
 */

__attribute__((naked, noreturn))
static void Boot_BranchToApp(
    uint32_t app_msp,
    uint32_t app_reset)
{
    __asm volatile
    (
        "msr msp, r0 \n"
        "cpsie i      \n"
        "bx r1        \n"
    );
}

void Boot_JumpToApp(uint32_t app_addr)
{
    uint32_t app_msp;
    uint32_t app_reset;

    /* 1. 检查应用程序向量表 */
    if (!Boot_CheckAppVector(app_addr))
    {
        return;
    }

    /* 2. 读取 Application 初始栈和入口 */
    app_msp =
        *(volatile const uint32_t *)app_addr;

    app_reset =
        *(volatile const uint32_t *)(app_addr + 4U);

    /*
     * 3. 此处需要确保：
     *
     * - 外设已经安全关闭
     * - 电机输出保持禁用
     * - DMA 已经停止
     *
     * 第一阶段尚未使用这些外设。
     */

    /* 4. 临时关闭可屏蔽中断 */
    __disable_irq();

    /* 5. 停止 SysTick */
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL  = 0U;

    /* 6. 清除 SysTick 和 PendSV 挂起状态 */
    SCB->ICSR =
        SCB_ICSR_PENDSTCLR_Msk |
        SCB_ICSR_PENDSVCLR_Msk;

    /* 7. 禁用并清除外部中断 */
    for (uint32_t i = 0; i < 8U; i++)
    {
        NVIC->ICER[i] = 0xFFFFFFFFU;
        NVIC->ICPR[i] = 0xFFFFFFFFU;
    }

    /* 8. 使用特权线程模式和 MSP */
    __set_CONTROL(0U);
    __ISB();

    /* 9. 设置应用程序中断向量表 */
    SCB->VTOR = app_addr;

    __DSB();
    __ISB();

    /* 10. 设置 MSP 并跳转 */
    Boot_BranchToApp(app_msp, app_reset);

    /* 不应该执行到这里 */
    while (1)
    {
    }
}
```

这个版本适合我们当前的**最小跳转实验**：Bootloader 从默认时钟启动，只执行 HAL 初始化，没有启用 UART、DMA、PWM、看门狗等其他功能。

以后给 Bootloader 增加 UART、Flash 和看门狗时，必须补全外设反初始化、时钟恢复及跳转前安全处理。不能在复杂外设已运行的情况下直接照搬这个版本。

另外，正常情况下 F411 的 NMI、HardFault 等异常不能像普通 NVIC 中断一样通过 ICER 全部关闭，因此仍需保证跳转过程不会有未处理的异常。

---

# 七、设计 Bank A 和 Bank B 应用程序

现在 Bootloader 已经有了跳转函数。

但它还没有能够跳转的 Application。

因此，接下来需要生成两个具有不同链接地址的程序。

注意：

**Bank A 和 Bank B 可以使用完全相同的 C 源码，但必须分别按照各自的 Flash 起始地址链接。**

## 7.1 Bank A 的链接脚本

创建：

```
app_A.ld
```

从 CubeMX 原本生成的 Application 链接脚本复制一份，只修改 `MEMORY`：

```
MEMORY
{
    RAM (xrw) :
        ORIGIN = 0x20000000,
        LENGTH = 128K

    FLASH (rx) :
        ORIGIN = 0x08010000,
        LENGTH = 192K
}
```

注意保留原链接脚本中的 `SECTIONS`、堆栈和其他定义。

这样就规定了：

```
Bank A 程序起始地址：

0x08010000
```

## 7.2 Bank B 的链接脚本

创建：

```
app_B.ld
```

修改：

```
MEMORY
{
    RAM (xrw) :
        ORIGIN = 0x20000000,
        LENGTH = 128K

    FLASH (rx) :
        ORIGIN = 0x08040000,
        LENGTH = 192K
}
```

这样规定：

```
Bank B 程序起始地址：

0x08040000
```

你会发现：

两个程序使用相同的 RAM。

为什么？

因为正常情况下，只有一个 Application 在运行。

```
Bootloader
    │
    ├── 跳转 Bank A
    │
    └── 跳转 Bank B
```

两份 App 不会同时执行，因此可以复用 SRAM。

## 7.3 修改 Application 的中断向量表

应用程序自己初始化时，也需要知道自己运行在哪个 Bank。

推荐通过编译宏定义：

```
#if defined(APP_BANK_A)

#define APP_VECTOR_ADDR 0x08010000U

#elif defined(APP_BANK_B)

#define APP_VECTOR_ADDR 0x08040000U

#else

#error "Application bank is not defined"

#endif
```

在 Application 的 `SystemInit()` 中确保正确设置：

```
SCB->VTOR = APP_VECTOR_ADDR;
```

具体需要根据你工程中 `system_stm32f4xx.c` 的现有实现调整。

因为某些 Cube 工程会在 `SystemInit()` 中设置 VTOR，如果你只在 `main()` 里修改，可能存在启动早期中断向量位置不正确的问题。

最终编译时：

```
APP_BANK_A
    +
app_A.ld
    ↓
app_A.bin
```

以及：

```
APP_BANK_B
    +
app_B.ld
    ↓
app_B.bin
```

因此，以后你的每个固件版本都可以生成：

```
flight_v1.0_A.bin
flight_v1.0_B.bin
```

后续由 ESP32 根据目标分区选择对应的固件。

---

# 八、第一次实验：让 Bootloader 启动 Bank A 和 Bank B

这是我们目前最重要的实验。

先不要使用你的真实四旋翼飞控程序。

创建一个最简单的 Application，用 LED 或 UART 输出来判断程序是否运行。

## 8.1 Application 测试代码

例如：

```
int main(void)
{
    HAL_Init();

    SystemClock_Config();

    MX_GPIO_Init();

    while (1)
    {
        HAL_GPIO_TogglePin(
            GPIOC,
            GPIO_PIN_13
        );

        HAL_Delay(500);
    }
}
```

这里假设你有连接到 PC13 的 LED；如果你的 PCB 没有这个连接，可以换成实际 LED 引脚，或者改用 UART 打印。

为了方便区分，可以让：

```
Bank A：每 500ms 翻转一次 LED

Bank B：每 100ms 翻转一次 LED
```

使用同一套源码，通过编译宏区分：

```
#if defined(APP_BANK_A)

#define BLINK_PERIOD 500U

#elif defined(APP_BANK_B)

#define BLINK_PERIOD 100U

#endif
```

循环中：

```
HAL_Delay(BLINK_PERIOD);
```

这样你可以通过观察 LED 闪烁速度来判断当前运行的是哪个 Bank。

## 8.2 Bootloader 主函数

首先测试 Bank A。

`Bootloader/Core/Src/main.c`：

```
#include "main.h"
#include "boot_config.h"
#include "boot_jump.h"

int main(void)
{
    HAL_Init();

    /*
     * 第一阶段保持默认 HSI 时钟。
     * 暂不初始化 UART、DMA 等外设。
     */

    Boot_JumpToApp(BANK_A_ADDR);

    /*
     * 如果 Application 无效，
     * 跳转函数会返回。
     *
     * 此时保持安全状态。
     */
    while (1)
    {
    }
}
```

现在你已经有三个可执行文件：

```
bootloader.elf

app_A.elf

app_B.elf
```

分别转换为 BIN：

```
arm-none-eabi-objcopy \
    -O binary \
    bootloader.elf \
    bootloader.bin
```

其他两个工程同理。

## 8.3 检查编译结果

这是我非常建议你养成的习惯。

在烧录之前，先检查 ELF 中的代码地址。

```
arm-none-eabi-objdump -h app_A.elf
```

找到：

```
.isr_vector
```

其地址应为：

```
0x08010000
```

对于 Bank B：

```
arm-none-eabi-objdump -h app_B.elf
```

应该看到：

```
.isr_vector
```

位于：

```
0x08040000
```

还可以查看：

```
arm-none-eabi-nm app_A.elf | grep Reset_Handler
```

这样可以确认 `Reset_Handler` 最终被链接到了哪里。

**这一步能帮助你理解链接脚本到底对程序产生了什么影响。**

---

# 九、使用 ST-Link 烧录三个程序

现在将三个程序写入指定地址。

建议使用 STM32CubeProgrammer。

|程序|烧录地址|
|---|---|
|bootloader.bin|`0x08000000`|
|app_A.bin|`0x08010000`|
|app_B.bin|`0x08040000`|

注意：每次烧录时都要选择正确的目标地址，不要随意执行 Full Chip Erase，否则已经写入的其他分区会被擦除。

第一次调试时，可以先擦除整个芯片，再依次写入三个程序。

完成以后：

```
0x08000000 → bootloader.bin

0x08010000 → app_A.bin

0x08040000 → app_B.bin
```

现在给 STM32 断电再上电。

预期：

```
STM32 上电
    │
    ▼
Bootloader
    │
    ▼
读取 Bank A 向量表
    │
    ▼
设置 MSP
    │
    ▼
设置 VTOR
    │
    ▼
跳转到 Bank A Reset_Handler
    │
    ▼
执行 Bank A main()
    │
    ▼
LED 按 500ms 周期翻转
```

如果这个过程成功，说明：

**Bootloader 已经能够正常启动 Bank A。**

接下来修改 Bootloader：

```
Boot_JumpToApp(BANK_B_ADDR);
```

重新编译并仅烧录 Bootloader。

再次上电。

如果 Bank B 能够正常运行，LED 按 100ms 周期翻转，就说明你已经具备了 A/B 双分区启动的基础。

这个阶段先在不安装螺旋桨的状态下调试，避免 Bootloader 和 Application 切换期间电机输出状态不确定。

---

# 十、完成程序跳转后，下一步是什么？

现在回顾我们刚刚完成的结构：

```
                 STM32F411
                     │
                     ▼
                Bootloader
                     │
                     ▼
               Jump_To_App()
                     │
             ┌───────┴───────┐
             │               │
             ▼               ▼
          Bank A          Bank B
             │               │
             ▼               ▼
         App A main()     App B main()
```

但目前还有一个明显的问题：

我们是通过修改源代码决定启动哪个 Bank 的。

```
Boot_JumpToApp(BANK_A_ADDR);
```

或者：

```
Boot_JumpToApp(BANK_B_ADDR);
```

而真正的 OTA 系统不应该每次手动修改程序。

因此，接下来就需要引入：

## Boot Metadata：启动管理信息

它是 A/B OTA 能够自动切换和回滚的核心。

假设初始状态：

```
Bank A：v1.0
Bank B：空

confirmed_bank = A
pending_bank   = NONE
```

升级时：

```
1. ESP32 发送 app_B.bin

2. Bootloader 擦除 Bank B

3. Bootloader 写入 Bank B

4. Bootloader 校验 Bank B

5. 修改 Metadata：

   confirmed_bank = A
   pending_bank   = B
   status         = PENDING
```

随后重新启动。

Bootloader 读取 Metadata：

```
status == PENDING
       │
       ▼
先将状态保存为 TRYING
       │
       ▼
启动 Bank B
```

如果 Bank B 正常启动：

```
Application B
       │
       ▼
完成初始化与自检
       │
       ▼
OTA_Confirm()
       │
       ▼
confirmed_bank = B
pending_bank   = NONE
status         = CONFIRMED
```

如果 Bank B 启动后发生 HardFault，并且看门狗触发复位：

```
Bootloader 重新启动
       │
       ▼
读取 Metadata
       │
       ▼
status == TRYING
       │
       ▼
新版本未完成确认
       │
       ▼
恢复 Bank A
```

这就是我们下一阶段要实现的完整逻辑。

### Bootloader 后续需要的核心模块

|源文件|功能|涉及的底层知识|
|---|---|---|
|`boot_jump.c`|程序跳转|MSP、PC、VTOR、NVIC|
|`boot_flash.c`|Flash 擦除与写入|FLASH_CR、FLASH_SR、Sector|
|`boot_metadata.c`|保存启动状态|Flash 持久化、掉电保护|
|`boot_manager.c`|Bank 选择与回滚|状态机、看门狗|
|`boot_protocol.c`|接收新固件|UART、CRC、超时重传|
|`boot_verify.c`|验证固件|CRC、哈希、固件有效性|

后续我们会将固件元数据设计为双副本、带序列号和 CRC 的结构，确保升级过程中断电时，仍然有机会找到上一次有效的启动记录。

对于长期使用的飞控，还应增加固件签名验证，避免仅凭 CRC 就允许执行未经授权的固件。

---

# 十一、这一阶段你应该掌握什么？

现在最重要的并不是急着实现 UART OTA，而是把下面几个底层概念弄清楚：

**第一个：CPU 上电时为什么从 Flash 向量表读取两个地址？**

你需要理解：

```
初始 MSP
Reset_Handler
```

**第二个：链接脚本如何决定程序在 Flash 中的位置？**

重点理解：

```
FLASH (rx) :
    ORIGIN = 0x08010000,
    LENGTH = 192K
```

以及：

```
.isr_vector :
{
    KEEP(*(.isr_vector))
} >FLASH
```

**第三个：Bootloader 为什么能够启动另一个独立编译的程序？**

重点理解：

```
SCB->VTOR = app_addr;
```

以及：

```
msr msp, r0
bx r1
```

**第四个：为什么需要在跳转前处理 SysTick、NVIC 和外设？**

因为 Application 必须拥有独立、可预期的运行环境。

### 建议阅读的官方资料

- [PM0214 — Cortex-M4 编程手册](https://www.st.com/resource/en/programming_manual/pm0214-stm32-cortexm4-mcus-and-mpus-programming-manual-stmicroelectronics.pdf?utm_source=chatgpt.com)：第 2.1 节讲 MSP、PSP 和寄存器，第 2.3.4 节讲向量表，第 4.4.4 节讲 VTOR。
- [RM0383 — STM32F411 参考手册](https://www.st.com/resource/en/reference_manual/dm00119316-stm32f411xce-advanced-armbased-32bit-mcus-stmicroelectronics.pdf?utm_source=chatgpt.com)：第 3 章讲 Flash 结构、擦除、编程及底层寄存器。
- [AN3965 — STM32 IAP 实现指南](https://www.st.com/resource/en/application_note/dm00036049-stm32f40x-stm32f41x-in-application-programming-using-the-usart-stmicroelectronics.pdf?utm_source=chatgpt.com)：讲解 Bootloader 下载、执行用户程序的基本架构。

---

## 本阶段的最终验收目标

你应该能够独立完成下面这个实验：

```
             STM32F411
                 │
                 ▼
             Bootloader
                 │
                 ▼
        读取 Application 向量表
                 │
                 ▼
          检查 MSP 和 PC
                 │
                 ▼
            设置 VTOR
                 │
                 ▼
         设置 MSP 并跳转
                 │
         ┌───────┴───────┐
         │               │
         ▼               ▼
      Bank A          Bank B
         │               │
         ▼               ▼
      LED 慢闪         LED 快闪
```

**只要实现了这个实验，我们就完成了 Bootloader 最底层的启动与跳转机制。**

下一阶段将进入真正的 Flash 底层开发：从 `FLASH_KEYR`、`FLASH_CR`、`FLASH_SR` 寄存器开始，理解 Flash 为什么需要解锁、扇区为什么必须先擦除，以及如何实现安全的 `Flash_EraseBank()` 和 `Flash_Write()`。

随后再设计 Metadata 状态机，把你现在写死的 Bank 选择，升级为能够自动切换、试运行和回滚的 A/B OTA 引导系统。