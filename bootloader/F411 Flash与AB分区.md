# STM32F411CEU6 Flash 与 A/B 分区

## 硬件参数

- 以 **STM32F411CEU6（512KB Flash、128KB SRAM）** 为例；实际应先检查芯片完整丝印。
- 单物理 Flash Bank；Sector 大小不均匀。

| Sector | 起始地址 | 大小 | 用途 |
|---|---|---:|---|
| 0 | `0x08000000` | 16KB | Bootloader |
| 1 | `0x08004000` | 16KB | Bootloader |
| 2 | `0x08008000` | 16KB | Metadata 0 |
| 3 | `0x0800C000` | 16KB | Metadata 1 |
| 4 | `0x08010000` | 64KB | Bank A |
| 5 | `0x08020000` | 128KB | Bank A |
| 6 | `0x08040000` | 128KB | Bank B |
| 7 | `0x08060000` | 128KB | Bank B |

## 横向分区示意（宽度不按实际比例）

```text
低地址                                                            高地址
┌────────────┬─────────┬─────────┬───────────────┬───────────────┐
│ Bootloader │ Meta 0  │ Meta 1  │    Bank A     │    Bank B     │
│   32KB     │  16KB   │  16KB   │     192KB     │     256KB     │
└────────────┴─────────┴─────────┴───────────────┴───────────────┘
0x08000000                      0x08010000      0x08040000       0x08080000
```

## 关键宏

```c
#define BOOT_ADDR     0x08000000U
#define META0_ADDR    0x08008000U
#define META1_ADDR    0x0800C000U
#define BANK_A_ADDR   0x08010000U
#define BANK_B_ADDR   0x08040000U
#define APP_MAX_SIZE  (192U * 1024U)
```

## 为什么这样分？

- Bootloader 占 Sector 0、1（32KB），需要**检查最终代码体积**。
- Metadata 用两个独立 Sector 做冗余记录，避免掉电后丢失唯一启动状态。
- Bank A 物理 192KB，Bank B 物理 256KB；双镜像统一限制 App ≤192KB。
- **擦除必须以 Sector 为单位**：擦除 B 的 Sector 7 会清空整个 128KB，不能把其中多出的 64KB 当独立擦除分区。
- 软件上的 Bank A/B **不是** MCU 的硬件 Bank Swap。

> [!important] 不要踩坑
> `.ld` 对 Flash 长度的限制不是硬件写保护；Bootloader 写入前仍需检查目标地址和长度。正式实现还可考虑硬件写保护。

关联：[[链接脚本与双镜像构建]]、[[Flash擦写与UART升级协议]]。
