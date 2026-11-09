// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2020 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "supervisor/board.h"
#include "mpconfigboard.h"
#include "driver/gpio.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/digitalio/DigitalInOut.h"
#include "shared-bindings/time/__init__.h"
#include "shared-module/displayio/__init__.h"
#include "shared-module/displayio/mipi_constants.h"
#include "shared-bindings/paralleldisplaybus/ParallelBus.h"
#include "shared-bindings/busdisplay/BusDisplay.h"

#define DELAY 0x80

// display init sequence according to LilyGo example app, in 320x240 landscape
uint8_t display_init_sequence[] = {
    // sw reset
    0x01, 0 | DELAY, 100,
    // sleep out
    0x11, 0 | DELAY, 255,
    // madctl display and color format settings (landscape)
    0x36, 1, 0xA0,
    // 16 bits per pixel
    0x3A, 1, 0x05,
    // porches
    0xB2, 5, 0x0C, 0x0C, 0x00, 0x33, 0x33,
    // gate control
    0xB7, 1, 0x35,
    // VCOM
    0xBB, 1, 0x28,
    // power control
    0xC0, 1, 0x2C,
    0xC2, 1, 0x01,
    0xC3, 1, 0x0B,
    0xC4, 1, 0x20,
    0xC6, 1, 0x0F,
    // interface control
    0xD0, 2, 0xA4, 0xA1,
    // ST7789 gamma settings
    0xE0, 14, 0xD0, 0x01, 0x08, 0x0F, 0x11, 0x2A, 0x36, 0x55, 0x44, 0x3A, 0x0B, 0x06, 0x11, 0x20,
    0xE1, 14, 0xD0, 0x02, 0x07, 0x0A, 0x0B, 0x18, 0x34, 0x43, 0x4A, 0x2B, 0x1B, 0x1C, 0x22, 0x1F,
    // display on
    0x29, 0 | DELAY, 255,
};

static void display_init(void) {
    // 先确保电源稳定
    common_hal_time_delay_ms(10);
    
    paralleldisplaybus_parallelbus_obj_t *bus = &allocate_display_bus()->parallel_bus;
    bus->base.type = &paralleldisplaybus_parallelbus_type;
    
    const mcu_pin_obj_t *data_pins[] = {
        &pin_GPIO48,  // D0
        &pin_GPIO47,  // D1
        &pin_GPIO39,  // D2
        &pin_GPIO40,  // D3
        &pin_GPIO41,  // D4
        &pin_GPIO42,  // D5
        &pin_GPIO45,  // D6
        &pin_GPIO46   // D7
    };
    
    common_hal_paralleldisplaybus_parallelbus_construct_nonsequential(
        bus,
        8, // num pins
        data_pins, // Data pins
        &pin_GPIO7,  // Command or data (DC)
        &pin_GPIO6,  // Chip select (CS)
        &pin_GPIO8,  // Write (WR)
        NULL,        // Read. Not connected on this board.
        NULL,        // Reset. The panel has no reset line, use SWRESET.
        5000000);    // Frequency. LilyGo runs the panel at 5 MHz.

    busdisplay_busdisplay_obj_t *display = &allocate_display()->display;
    display->base.type = &busdisplay_busdisplay_type;
    
    common_hal_busdisplay_busdisplay_construct(
        display,
        bus,
        320, // Width
        240, // Height
        0,     // column start
        0,     // row start
        0,     // rotation
        16,    // Color depth
        false, // grayscale
        false, // pixels_in_byte_share_row (unused for depths > 8)
        1,     // bytes per cell. Only valid for depths < 8
        false
