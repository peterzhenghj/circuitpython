// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2020 Lucian Copeland for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#pragma once

#include "stm32f4xx_hal.h"

// Chip:                STM32F429ZI
// Line Type:           Performance Line
// Speed:               180MHz (MAX)

// Note: STM32F429 uses external 8MHz HSE via ST-LINK MCO (see UM1670 §6.12.1).
// USB requires 48MHz clock, derived from PLLQ or PLLSAI (Audio PLL).

// Defaults:
#ifndef CPY_CLK_VSCALE
#define CPY_CLK_VSCALE (PWR_REGULATOR_VOLTAGE_SCALE1)
#endif

#ifndef CPY_CLK_PLLN
#define CPY_CLK_PLLN (360)  // (8MHz / PLLM=8) * 360 = 360MHz VCO
#endif

#ifndef CPY_CLK_PLLP
#define CPY_CLK_PLLP (RCC_PLLP_DIV2)  // SYSCLK = 180MHz
#endif

#ifndef CPY_CLK_PLLQ
#define CPY_CLK_PLLQ (7)  // 360 / 7 = ~51.4MHz for USB
#endif

#ifndef CPY_CLK_AHBDIV
#define CPY_CLK_AHBDIV (RCC_SYSCLK_DIV1)
#endif

#ifndef CPY_CLK_APB1DIV
#define CPY_CLK_APB1DIV (RCC_HCLK_DIV4)  // Max 45MHz
#endif

#ifndef CPY_CLK_APB2DIV
#define CPY_CLK_APB2DIV (RCC_HCLK_DIV2)  // Max 90MHz
#endif

#ifndef CPY_CLK_FLASH_LATENCY
#define CPY_CLK_FLASH_LATENCY (FLASH_LATENCY_5)  // Required for 180MHz
#endif

#ifndef CPY_CLK_USB_USES_AUDIOPLL
#define CPY_CLK_USB_USES_AUDIOPLL (1)  // Use PLLSAI for USB 48MHz
#endif

#ifndef BOARD_HSE_SOURCE
#define BOARD_HSE_SOURCE (RCC_HSE_BYPASS)  // ST-LINK MCO provides 8MHz
#endif

