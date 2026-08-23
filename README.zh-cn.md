# stm32-learning-05-oled

STM32 练习代码 #05：操作 OLED 屏幕。

[English](./README.md) | [中文](./README.zh-cn.md)

学习如何在开发板上使用 OLED 屏幕。

## 概述

本项目使用 u8g2 库驱动由 CH1116 芯片控制的 OLED 屏幕，并在其上显示温湿度信息。

<div>
  <table width="100%" cellspacing="2" cellpadding="0" border="0" cellpadding="0" frame="void">
    <tr>
      <td width="50%"><img src="./docs/img/img03.jpg" width="100%" alt="img03"></td>
      <td width="50%"><img src="./docs/img/img04.jpg" width="100%" alt="img04"></td>
    </tr>
  </table>
  <table width="100%" cellspacing="2" cellpadding="0" border="0" cellpadding="0" frame="void">
    <tr>
      <td width="25%"><img src="./docs/img/img03.jpg" width="100%" alt="img03"></td>
      <td width="25%"><img src="./docs/img/img04.jpg" width="100%" alt="img04"></td>
      <td width="25%"><img src="./docs/img/img05.jpg" width="100%" alt="img05"></td>
      <td width="25%"><img src="./docs/img/img06.jpg" width="100%" alt="img06"></td>
    </tr>
  </table>
</div>

屏幕第一行显示**示例文本**，按下 `Key2` 切换语言。第二行显示环境**温度**，按下 `Key1` 切换温标。第三行显示环境**相对湿度**，第四行以十六进制显示当前 **tick 计数**。红色状态灯闪烁周期为 2 秒，用于指示程序在非阻塞状态下运行。

本项目是 [Keysking 的 STM32 教程](https://www.bilibili.com/video/BV19u4y197df) 的配套练习，运行在 [他的开发板](https://docs.keysking.com/docs/stm32/resourcePack/) 上。（其实代码实现和他讲的没什么关系。）

## 特点

- 基于 STM32 HAL 库、使用 C++17 编写的裸机程序。
- 完全非阻塞：基于 DMA 的异步 I2C；每个模块都是基于 tick 驱动的状态机。
- 面向对象，内置通用组件（状态机、RAII 中断保护、实例注册表），便于移植和扩展。

## 实现细节

### 芯片配置

- **时钟**
  - **RCC > HSE**：晶体/陶瓷谐振器
  - **HCLK**：72 MHz
- **I2C1**（CH1116 OLED、AHT20）
  - **速度模式**：快速模式（400 kHz）
  - **DMA**：TX 使用 DMA1_CH6，RX 使用 DMA1_CH7
- **GPIO**
  - **PA6/PA7/PB0**：RGB LED
  - **PB12/PB13**：Key1 / Key2

### 编程

每个模块都是一个类，其 `loop()` 是由 `HAL_GetTick()` 驱动的非阻塞状态机，全部由 `cpp_loop()` 调用：

- `utils` —— 通用构建模块：`State<E>`（基于 tick 的状态机节点）、`IrqGuard`（RAII 中断屏蔽）、`InstanceRegistry<T>`（CRTP 链表注册表，使 `loop()` 可以遍历所有实例）。
- `i2c::Bus` —— 基于 HAL DMA 传输的异步 I2C，带有任务队列（`write` / `read` / `memWrite` / `memRead`）；HAL 中断回调将完成状态分发给对应的任务钩子。
- `aht20` —— 非阻塞 AHT20 驱动：状态机（初始化 → 触发 → 等待 → 读取），带 CRC8 校验；结果保存在 `tp` / `rh` 中。基本复用 [stm32-learning-04-i2c](https://github.com/limpidautumn/stm32-learning-04-i2c) 的代码。
- `key::Key` —— 按键驱动，带边沿检测和 10 ms 消抖；提供下降沿/上升沿钩子。
- `oled` —— 通过自定义字节回调（`U8X8_WITH_USER_PTR`）将 u8g2 接入异步 I2C 总线；128×64 帧缓冲每 40 ms 刷新一次；多语言文本使用内嵌的 `u8g2_font_unifont_hello` 子集字体渲染；`drawRev()` 实现滚动反色带的动画效果，防止烧屏。
- `led::StatusLed` —— 以 2s 周期闪烁红色 LED。
- `app_main` —— `cpp_setup()` / `cpp_loop()`，由 CubeMX 生成的 `main.c` 调用的入口点。

屏幕（CH1116）通过 u8g2 的 SSD1309 兼容初始化序列（`u8x8_d_ssd1309_128x64_noname2`）进行初始化。

## 目录结构

```
stm32-learning-05-oled
├───CMakeLists.txt
├───README.md
├───stm32-learning-05-oled.ioc
├───Core
│   ├───Inc
│   │   ├───aht20.hpp
│   │   ├───app_main.hpp
│   │   ├───i2c.hpp
│   │   ├───key.hpp
│   │   ├───led.hpp
│   │   ├───main.h
│   │   ├───oled.hpp
│   │   ├───u8g2_font_unifont_hello.h
│   │   └───utils.hpp
│   └───Src
│       ├───aht20.cpp
│       ├───app_main.cpp
│       ├───i2c.cpp
│       ├───key.cpp
│       ├───led.cpp
│       ├───main.c
│       ├───oled.cpp
│       └───u8g2_font_unifont_hello.c
└───Drivers
```

仅列出用户编写的文件；HAL/CMSIS 库及 CubeMX 生成的源码（`dma.c`、`gpio.c`、`i2c.c`、`usart.c` 等）已省略。

## 收获

- 使用 u8g2 库驱动 OLED 屏幕，并为其接入自定义的异步 I2C 传输层。
- 非阻塞设计：使用 DMA I2C + 状态机，代替阻塞式 IRQ 等待。
- 裸机上的 C++：CRTP 实例注册表、RAII 保护、通用状态机。
- AHT20 温湿度传感器：测量协议与 CRC8 校验。
- 按键消抖与边沿检测。
- 使用 Unicode 子集字体显示多语言文本。
