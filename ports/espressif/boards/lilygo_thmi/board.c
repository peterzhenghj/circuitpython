// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2020 Scott Shawcroft for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "supervisor/board.h"
#include "mpconfigboard.h"
#include "driver/gpio.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/time/__init__.h"
#include "shared-module/displayio/__init__.h"
#include "shared-module/displayio/mipi_constants.h"
#include "shared-bindings/paralleldisplaybus/ParallelBus.h"
#include "shared-bindings/busdisplay/BusDisplay.h"

#define DELAY 0x80

// ST7789 initialization sequence for 320x240 landscape
// Reference: ST7789 datasheet + LilyGo T-HMI official example
uint8_t display_init_sequence[] = {
    // Software reset - must wait 150ms+ after this command
    0x01, 0 | DELAY, 150,
    // Sleep out - must wait 120ms+ (ST7789 datasheet requirement)
    0x11, 0 | DELAY, 120,
    // MADCTL: Memory Access Control
    // 0xA0 = 1010 0000
    //        |||| ||||-- D7: MY (Row Address Order) = 1
    //        |||| |||--- D6: MX (Column Address Order) = 0
    //        |||| ||---- D5: MV (Row/Column Exchange) = 1 (landscape)
    //        |||| |----- D4: ML (Vertical Refresh Order) = 0
    //        ||||------- D3: BGR/RGB = 0 (RGB)
    //        |||-------- D2: MH (Horizontal Refresh Order) = 0
    0x36, 1, 0xA0,
    // COLMOD: Interface Pixel Format - 16 bits/pixel (RGB565)
    0x3A, 1, 0x05,
    // PORCTRL: Porch Setting
    0xB2, 5, 0x0C, 0x0C, 0x00, 0x33, 0x33,
    // GCTRL: Gate Control
    0xB7, 1, 0x35,
    // VCOMS: VCOM Setting
    0xBB, 1, 0x28,
    // LCMCTRL: LCM Control
    0xC0, 1, 0x2C,
    // VDVVRHEN: VDV and VRH Command Enable
    0xC2, 1, 0x01,
    // VRHS: VRH Set
    0xC3, 1, 0x0B,
    // VDVS: VDV Set
    0xC4, 1, 0x20,
    // FRCTRL2: Frame Rate Control in Normal Mode
    0xC6, 1, 0x0F,
    // PWCTRL1: Power Control 1
    0xD0, 2, 0xA4, 0xA1,
    // PVGAMCTRL: Positive Voltage Gamma Control
    0xE0, 14, 0xD0, 0x01, 0x08, 0x0F, 0x11, 0x2A, 0x36, 0x55, 0x44, 0x3A, 0x0B, 0x06, 0x11, 0x20,
    // NVGAMCTRL: Negative Voltage Gamma Control
    0xE1, 14, 0xD0, 0x02, 0x07, 0x0A, 0x0B, 0x18, 0x34, 0x43, 0x4A, 0x2B, 0x1B, 0x1C, 0x22, 0x1F,
    // Display ON - wait 100ms+
    0x29, 0 | DELAY, 100,
};

static void display_init(void) {
    // Ensure power is stable before initializing display
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
        NULL,        // Reset. No hardware reset pin on T-HMI, use software reset (0x01).
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
        false, // reverse_pixels_in_byte. Only valid for depths < 8
        true,  // reverse_pixels_in_word
        MIPI_COMMAND_SET_COLUMN_ADDRESS, // Set column command
        MIPI_COMMAND_SET_PAGE_ADDRESS,   // Set row command
        MIPI_COMMAND_WRITE_MEMORY_START, // Write memory command
        display_init_sequence,
        sizeof(display_init_sequence),
        &pin_GPIO38, // Backlight pin
        NO_BRIGHTNESS_COMMAND,
        1.0f,        // brightness
        false,       // single_byte_bounds
        false,       // data_as_commands
        true,        // auto_refresh
        60,          // native_frames_per_second
        true,        // backlight_on_high
        false,       // SH1107_addressing
        50000);      // backlight pwm frequency
}

void board_init(void) {
    // GPIO14 (PWR_ON) and GPIO10 (PWR_EN) both hold the display powered on.
    espressif_board_reset_pin_number(14);
    common_hal_time_delay_ms(5);
    espressif_board_reset_pin_number(10);

    // Display
    display_init();
}

bool espressif_board_reset_pin_number(gpio_num_t pin_number) {
    // Keep the display power pins enabled.
    if (pin_number == 14 || pin_number == 10) {
        config_pin_as_output_with_level(pin_number, true);
        return true;
    }
    return false;
}

// Use the MP_WEAK supervisor/shared/board.c versions of routines not defined here.
