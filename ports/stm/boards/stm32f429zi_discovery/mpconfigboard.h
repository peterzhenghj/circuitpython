// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2019 Lucian Copeland for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#pragma once

// CircuitPython 配置

#define MICROPY_HW_BOARD_NAME       "STM32F429DISC"
#define MICROPY_HW_MCU_NAME         "STM32F429ZI"

#define FLASH_SIZE                  (0x200000)   // 2MB Flash
#define FLASH_PAGE_SIZE             (0x20000)    // 128KB per sector

#define HSE_VALUE ((uint32_t)8000000)           // 外部 HSE 频率（来自 ST-LINK MCO）
#define LSE_VALUE ((uint32_t)32768)             // 外部 32.768 kHz 晶振
#define BOARD_HSE_SOURCE           (RCC_HSE_BYPASS)
#define BOARD_HAS_LOW_SPEED_CRYSTAL (1)
#define CPY_CLK_USB_USES_AUDIOPLL   (1)

// 默认 I2C 接口（基于原理图，PB8/PB9 连接至扩展接口）
#define DEFAULT_I2C_BUS_SCL (&pin_PB08)
#define DEFAULT_I2C_BUS_SDA (&pin_PB09)

// 默认 UART，用于 Virtual COM Port（通过 ST-LINK/V2-B）
#define DEFAULT_UART_BUS_RX (&pin_PA10)
#define DEFAULT_UART_BUS_TX (&pin_PA09)

// 板载 LED 灯（LD3：绿色 PG13）
#define MICROPY_HW_LED_STATUS (&pin_PG13)
#define MICROPY_HW_LED_STATUS_INVERTED (0)

// 板载用户按键（B1：PA0）
#define MICROPY_HW_BUTTON_USER (&pin_PA0)

#define MICROPY_HW_LED1       (&pin_PG13)
#define MICROPY_HW_LED2       (&pin_PG14)
#define MICROPY_HW_BUTTON     (&pin_PA0)


