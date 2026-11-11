// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2026
//
// SPDX-License-Identifier: MIT

#include "supervisor/board.h"
#include "mpconfigboard.h"
#include "driver/gpio.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-module/displayio/__init__.h"
#include "shared-module/displayio/mipi_constants.h"

#define DELAY 0x80

#define PWR_EN_PIN (10)
#define PWR_ON_PIN (14)

static const uint8_t display_init_sequence[] = {
    0x01, 0 | DELAY, 150,
    0x11, 0 | DELAY, 150,
    0x13, 0,
    0x36, 1, 0x60,
    0xB6, 2, 0x0A, 0x82,
    0x3A, 1 | DELAY, 0x55, 10,
    0xB2, 5, 0x0C, 0x0C, 0x00, 0x33, 0x33,
    0xB7, 1, 0x35,
    0xBB, 1, 0x28,
    0xC0, 1, 0x0C,
    0xC2, 2, 0x01, 0xFF,
    0xC3, 1, 0x10,
    0xC4, 1, 0x20,
    0xC6, 1, 0x0F,
    0xD0, 2, 0xA4, 0xA1,
    0xE0, 14, 0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28, 0x32, 0x44, 0x42, 0x06, 0x0E, 0x12, 0x14, 0x17,
    0xE1, 14, 0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28, 0x31, 0x54, 0x47, 0x0E, 0x1C, 0x17, 0x1B, 0x1E,
    0x2A, 4, 0x00, 0x00, 0x01, 0x3F,
    0x2B, 4, 0x00, 0x00, 0x00, 0xEF,
    0x21, 0,
    0x29, 0 | DELAY, 150,
};

static void display_init(void) {
    paralleldisplaybus_parallelbus_obj_t *bus = &allocate_display_bus()->parallel_bus;
    bus->base.type = &paralleldisplaybus_parallelbus_type;
    const mcu_pin_obj_t *data_pins[] = {
        &pin_GPIO48,
        &pin_GPIO47,
        &pin_GPIO39,
        &pin_GPIO40,
        &pin_GPIO41,
        &pin_GPIO42,
        &pin_GPIO45,
        &pin_GPIO46
    };
    common_hal_paralleldisplaybus_parallelbus_construct_nonsequential(bus,
        8,
        data_pins,
        &pin_GPIO7,
        &pin_GPIO6,
        &pin_GPIO8,
        NULL,
        NULL,
        15000000);

    busdisplay_busdisplay_obj_t *display = &allocate_display()->display;
    display->base.type = &busdisplay_busdisplay_type;
    common_hal_busdisplay_busdisplay_construct(display,
        bus,
        320,
        240,
        0,
        0,
        0,
        16,
        false,
        false,
        1,
        false,
        true,
        MIPI_COMMAND_SET_COLUMN_ADDRESS,
        MIPI_COMMAND_SET_PAGE_ADDRESS,
        MIPI_COMMAND_WRITE_MEMORY_START,
        display_init_sequence,
        sizeof(display_init_sequence),
        &pin_GPIO38,
        NO_BRIGHTNESS_COMMAND,
        1.0f,
        false,
        false,
        true,
        60,
        true,
        false,
        50000);
}

void board_init(void) {
    espressif_board_reset_pin_number(PWR_ON_PIN);
    espressif_board_reset_pin_number(PWR_EN_PIN);
    display_init();
}

bool espressif_board_reset_pin_number(gpio_num_t pin_number) {
    if (pin_number == PWR_EN_PIN || pin_number == PWR_ON_PIN) {
        config_pin_as_output_with_level(pin_number, true);
        return true;
    }
    return false;
}

// Use the MP_WEAK supervisor/shared/board.c versions of routines not defined here.