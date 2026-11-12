// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2026
//
// SPDX-License-Identifier: MIT

#include "supervisor/board.h"
#include "mpconfigboard.h"
#include "driver/gpio.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "common-hal/microcontroller/Pin.h"

#define PWR_EN_PIN (10)
#define PWR_ON_PIN (14)

void board_init(void) {
    config_pin_as_output_with_level(PWR_ON_PIN, true);
    config_pin_as_output_with_level(PWR_EN_PIN, true);
}

bool espressif_board_reset_pin_number(gpio_num_t pin_number) {
    if (pin_number == PWR_EN_PIN || pin_number == PWR_ON_PIN) {
        config_pin_as_output_with_level(pin_number, true);
        return true;
    }
    return false;
}

// Use the MP_WEAK supervisor/shared/board.c versions of routines not defined here.
