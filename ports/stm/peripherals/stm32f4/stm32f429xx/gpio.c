// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2019 Lucian Copeland for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "peripherals/gpio.h"
#include "common-hal/microcontroller/Pin.h"

void stm32_peripherals_gpio_init(void) {
    // 启用所有使用到的 GPIO 端口
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    // 保留调试和关键功能引脚
    never_reset_pin_number(2, 13); // PC13 - Anti-tamper
    never_reset_pin_number(2, 14); // PC14 - OSC32_IN
    never_reset_pin_number(2, 15); // PC15 - OSC32_OUT

    never_reset_pin_number(0, 13); // PA13 - SWDIO
    never_reset_pin_number(0, 14); // PA14 - SWCLK

    // 可选：调试口引脚（JTAG），如果未启用可注释掉
    // never_reset_pin_number(0, 15); // PA15 - JTDI
    // never_reset_pin_number(1, 3);  // PB3  - JTDO
    // never_reset_pin_number(1, 4);  // PB4  - JTRST

    // 如果你启用了 PH0 和 PH1 作为 HSE 外部时钟输入，也可考虑保留
    // never_reset_pin_number(7, 0); // PH0 - OSC_IN (HSE)
    // never_reset_pin_number(7, 1); // PH1 - OSC_OUT (HSE)
}
