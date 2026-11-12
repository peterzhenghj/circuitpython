// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2026
//
// SPDX-License-Identifier: MIT

#pragma once

// Micropython setup

#define MICROPY_HW_BOARD_NAME       "LILYGO T-HMI"
#define MICROPY_HW_MCU_NAME         "ESP32S3"

#define CIRCUITPY_BOARD_SPI (1)
#define CIRCUITPY_BOARD_SPI_PIN {{.clock = &pin_GPIO1, .mosi = &pin_GPIO3, .miso = &pin_GPIO4}, }

// The default SPI bus is the one shared with the XPT2046 touch controller.
#define DEFAULT_SPI_BUS_SCK         (&pin_GPIO1)
#define DEFAULT_SPI_BUS_MOSI        (&pin_GPIO3)
#define DEFAULT_SPI_BUS_MISO        (&pin_GPIO4)
