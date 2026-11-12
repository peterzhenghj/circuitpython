// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2026
//
// SPDX-License-Identifier: MIT

#include "supervisor/board.h"
#include "mpconfigboard.h"
#include "py/mphal.h"
#include "driver/gpio.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "common-hal/microcontroller/Pin.h"
#include "shared-module/displayio/__init__.h"
#include "shared-module/displayio/mipi_constants.h"

#define DELAY 0x80

#define PWR_EN_PIN (10)
#define PWR_ON_PIN (14)

static uint8_t display_init_sequence[] = {
    0x01, 0 | DELAY, 150,
    0x11, 0 | DELAY, 120,
    0x3A, 1, 0x55,
    0x36, 1, 0x60,
    0x20, 0 | DELAY, 10,
    0x13, 0 | DELAY, 10,
    0x29, 0 | DELAY, 100,
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
        10000000);

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
        false,
        MIPI_COMMAND_SET_COLUMN_ADDRESS,
        MIPI_COMMAND_SET_PAGE_ADDRESS,
        MIPI_COMMAND_WRITE_MEMORY_START,
        display_init_sequence,
        sizeof(display_init_sequence),
        NULL,
        NO_BRIGHTNESS_COMMAND,
        1.0f,
        false,
        false,
        false,
        60,
        true,
        false,
        50000);
}

void board_init(void) {
    config_pin_as_output_with_level(PWR_ON_PIN, true);
    config_pin_as_output_with_level(PWR_EN_PIN, true);
    mp_hal_delay_ms(500);
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
