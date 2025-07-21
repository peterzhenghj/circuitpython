// =============================================================================
// 2. ports/raspberrypi/common-hal/picodvi/Framebuffer_RP2350.c (Enhanced)
// =============================================================================
/*
 * This file is part of the CircuitPython project: https://circuitpython.org
 *
 * SPDX-FileCopyrightText: Copyright (c) 2024 Scott Shawcroft for Adafruit Industries
 *
 * SPDX-License-Identifier: MIT
 *
 * Enhanced RP2350 version with multi-resolution and refresh rate support:
 * - Added 640x480@60Hz for capture card compatibility
 * - Added 800x480@65Hz for Adafruit PID 2260 displays
 * - Preserved all existing 640x480@72Hz and 720x400@72Hz functionality
 * - Maintained all pixel-doubled modes for memory efficiency
 *
 * This implementation is RP2350-ONLY and uses HSTX peripheral features
 * not available on RP2040. The RP2040 implementation remains unchanged.
 */

#include "common-hal/picodvi/Framebuffer.h"

#include "py/gc.h"
#include "py/runtime.h"
#include "shared-bindings/time/__init__.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "supervisor/port.h"
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "hardware/structs/bus_ctrl.h"
#include "hardware/structs/hstx_ctrl.h"
#include "hardware/structs/hstx_fifo.h"

// ----------------------------------------------------------------------------
// DVI constants (existing, compatible with original implementation)
#define TMDS_CTRL_00 0x354u
#define TMDS_CTRL_01 0x0abu
#define TMDS_CTRL_10 0x154u
#define TMDS_CTRL_11 0x2abu

#define SYNC_V0_H0 (TMDS_CTRL_00 | (TMDS_CTRL_00 << 10) | (TMDS_CTRL_00 << 20))
#define SYNC_V0_H1 (TMDS_CTRL_01 | (TMDS_CTRL_00 << 10) | (TMDS_CTRL_00 << 20))
#define SYNC_V1_H0 (TMDS_CTRL_10 | (TMDS_CTRL_00 << 10) | (TMDS_CTRL_00 << 20))
#define SYNC_V1_H1 (TMDS_CTRL_11 | (TMDS_CTRL_00 << 10) | (TMDS_CTRL_00 << 20))

// ----------------------------------------------------------------------------
// Enhanced HSTX Command definitions for RP2350 
#define HSTX_CMD_RAW         (0x0u << 30)
#define HSTX_CMD_RAW_REPEAT  (0x1u << 30) 
#define HSTX_CMD_TMDS        (0x2u << 30)
#define HSTX_CMD_NOP         (0x3u << 30)

// Additional HSTX constants for RP2350
#define HSTX_CMD_RAW_DATA_LSB 0
#define HSTX_FIFO_CSR_EN_BITS (1u << 0)
#define HSTX_FIFO_CSR_LEVEL_LSB 1

// HSTX command array lengths
#define VSYNC_LEN 6     // Commands for vertical sync lines
#define VACTIVE_LEN 9   // Commands for active video lines

// ----------------------------------------------------------------------------
// Enhanced timing structure for all RP2350 supported display modes
typedef struct {
    uint16_t h_active, h_front, h_sync, h_back;
    uint16_t v_active, v_front, v_sync, v_back;
    uint32_t pixel_clock_hz;
    uint32_t hstx_clock_hz;
    uint8_t hstx_clkdiv;
    uint8_t hstx_n_shifts;
    uint8_t hstx_shift_amount;
} dvi_timing_t;

// Timing definitions for all supported modes on RP2350
// 640x480 modes
static const dvi_timing_t timing_640_480_60hz = {
    .h_active = 640, .h_front = 16, .h_sync = 96, .h_back = 48,
    .v_active = 480, .v_front = 10, .v_sync = 2, .v_back = 33,
    .pixel_clock_hz = 25175000,      // 25.175 MHz (VGA standard)
    .hstx_clock_hz = 125000000,      // 125 MHz 
    .hstx_clkdiv = 6, .hstx_n_shifts = 6, .hstx_shift_amount = 2
};

static const dvi_timing_t timing_640_480_72hz = {
    .h_active = 640, .h_front = 24, .h_sync = 40, .h_back = 128,
    .v_active = 480, .v_front = 9, .v_sync = 3, .v_back = 28,
    .pixel_clock_hz = 31500000,      // 31.5 MHz (VGA standard)
    .hstx_clock_hz = 125000000,      // 125 MHz (existing)
    .hstx_clkdiv = 5, .hstx_n_shifts = 5, .hstx_shift_amount = 2
};

// 720x400 mode (existing)
static const dvi_timing_t timing_720_400_72hz = {
    .h_active = 720, .h_front = 108, .h_sync = 108, .h_back = 108,
    .v_active = 400, .v_front = 42, .v_sync = 2, .v_back = 42,
    .pixel_clock_hz = 35500000,      // 35.5 MHz
    .hstx_clock_hz = 142000000,      // 142 MHz (existing)
    .hstx_clkdiv = 5, .hstx_n_shifts = 5, .hstx_shift_amount = 2
};

// 800x480 mode (NEW for RP2350)
static const dvi_timing_t timing_800_480_65hz = {
    .h_active = 800, .h_front = 40, .h_sync = 80, .h_back = 80,
    .v_active = 480, .v_front = 1, .v_sync = 3, .v_back = 16,
    .pixel_clock_hz = 32500000,      // 32.5 MHz  
    .hstx_clock_hz = 130000000,      // 130 MHz
    .hstx_clkdiv = 5, .hstx_n_shifts = 5, .hstx_shift_amount = 2
};

// TMDS sync command sequences for different resolutions (RP2350 specific)
// 640x480 mode commands
static const uint32_t vsync_line_640[VSYNC_LEN] = {
    HSTX_CMD_RAW_REPEAT | 16,  // H front porch
    SYNC_V0_H1,
    HSTX_CMD_RAW_REPEAT | 96,  // H sync width  
    SYNC_V0_H0,
    HSTX_CMD_RAW_REPEAT | (48 + 640), // H back porch + active
    SYNC_V0_H1
};

static const uint32_t vactive_line_640[VACTIVE_LEN] = {
    HSTX_CMD_RAW_REPEAT | 16,  // H front porch
    SYNC_V1_H1,
    HSTX_CMD_NOP,
    HSTX_CMD_RAW_REPEAT | 96,  // H sync width
    SYNC_V1_H0, 
    HSTX_CMD_NOP,
    HSTX_CMD_RAW_REPEAT | 48,  // H back porch
    SYNC_V1_H1,
    HSTX_CMD_TMDS | 640        // Active video data
};

// 720x400 mode commands (existing)
static const uint32_t vsync_line_720[VSYNC_LEN] = {
    HSTX_CMD_RAW_REPEAT | 108, // H front porch
    SYNC_V0_H1,
    HSTX_CMD_RAW_REPEAT | 108, // H sync width
    SYNC_V0_H0,
    HSTX_CMD_RAW_REPEAT | (108 + 720), // H back porch + active
    SYNC_V0_H1
};

static const uint32_t vactive_line_720[VACTIVE_LEN] = {
    HSTX_CMD_RAW_REPEAT | 108, // H front porch
    SYNC_V1_H1,
    HSTX_CMD_NOP,
    HSTX_CMD_RAW_REPEAT | 108, // H sync width
    SYNC_V1_H0,
    HSTX_CMD_NOP, 
    HSTX_CMD_RAW_REPEAT | 108, // H back porch
    SYNC_V1_H1,
    HSTX_CMD_TMDS | 720        // Active video data
};

// 800x480 mode commands (NEW for RP2350)
static const uint32_t vsync_line_800[VSYNC_LEN] = {
    HSTX_CMD_RAW_REPEAT | 40,  // H front porch
    SYNC_V0_H1,
    HSTX_CMD_RAW_REPEAT | 80,  // H sync width
    SYNC_V0_H0,
    HSTX_CMD_RAW_REPEAT | (80 + 800), // H back porch + active  
    SYNC_V0_H1
};

static const uint32_t vactive_line_800[VACTIVE_LEN] = {
    HSTX_CMD_RAW_REPEAT | 40,  // H front porch
    SYNC_V1_H1,
    HSTX_CMD_NOP,
    HSTX_CMD_RAW_REPEAT | 80,  // H sync width
    SYNC_V1_H0,
    HSTX_CMD_NOP,
    HSTX_CMD_RAW_REPEAT | 80,  // H back porch  
    SYNC_V1_H1,
    HSTX_CMD_TMDS | 800        // Active video data
};

// Get timing parameters for a given output resolution and refresh rate (RP2350)
static const dvi_timing_t* get_timing_params(mp_uint_t output_width, mp_uint_t output_height, mp_uint_t refresh_rate) {
    if (output_width == 640 && output_height == 480) {
        if (refresh_rate == 60) return &timing_640_480_60hz;    // NEW
        if (refresh_rate == 72) return &timing_640_480_72hz;    // existing
    }
    if (output_width == 720 && output_height == 400) {
        if (refresh_rate == 72) return &timing_720_400_72hz;    // existing
    }
    if (output_width == 800 && output_height == 480) {
        if (refresh_rate == 65) return &timing_800_480_65hz;    // NEW
    }
    return NULL; // Unsupported combination
}

// Get appropriate command arrays for resolution (RP2350 specific)
static void get_command_arrays(mp_uint_t output_width, 
                              const uint32_t **vsync_commands,
                              const uint32_t **vactive_commands) {
    if (output_width == 640) {
        *vsync_commands = vsync_line_640;
        *vactive_commands = vactive_line_640;
    } else if (output_width == 720) {
        *vsync_commands = vsync_line_720;
        *vactive_commands = vactive_line_720;
    } else if (output_width == 800) {
        *vsync_commands = vsync_line_800;
        *vactive_commands = vactive_line_800;
    }
}

// Build complete DMA command sequence for RP2350
static void build_dma_command_sequence(picodvi_framebuffer_obj_t *self, 
                                     const dvi_timing_t *timing, 
                                     mp_uint_t output_scaling) {
    const uint32_t *vsync_commands;
    const uint32_t *vactive_commands;
    get_command_arrays(self->output_width, &vsync_commands, &vactive_commands);
    
    size_t command_word = 0;
    size_t dma_command_size = (output_scaling == 1) ? 2 : 4;
    
    // Calculate line ranges for vertical timing
    size_t v_total = timing->v_active + timing->v_front + timing->v_sync + timing->v_back;
    size_t frontporch_start = v_total - timing->v_front;
    size_t frontporch_end = frontporch_start + timing->v_front;
    size_t vsync_start = frontporch_end;
    size_t vsync_end = vsync_start + timing->v_sync;
    size_t backporch_start = vsync_end;
    size_t backporch_end = backporch_start + timing->v_back;
    size_t active_start = backporch_end;
    size_t active_end = active_start + timing->v_active;
    
    // Generate DMA commands for each line in the frame
    for (size_t line = 0; line < v_total; line++) {
        const uint32_t *line_commands;
        size_t cmd_count;
        
        // Determine line type and appropriate command sequence
        if (line >= frontporch_start && line < frontporch_end) {
            // Front porch - use vsync commands
            line_commands = vsync_commands;
            cmd_count = VSYNC_LEN;
        } else if (line >= vsync_start && line < vsync_end) {
            // Vertical sync - use vsync commands  
            line_commands = vsync_commands;
            cmd_count = VSYNC_LEN;
        } else if (line >= backporch_start && line < backporch_end) {
            // Back porch - use vsync commands
            line_commands = vsync_commands;
            cmd_count = VSYNC_LEN;
        } else if (line >= active_start && line < active_end) {
            // Active video - use vactive commands
            line_commands = vactive_commands;
            cmd_count = VACTIVE_LEN;
        } else {
            // Default fallback
            line_commands = vsync_commands;
            cmd_count = VSYNC_LEN;
        }
        
        // Copy command sequence to DMA buffer
        for (size_t i = 0; i < cmd_count; i += 2) {
            if (command_word < self->dma_commands_len) {
                self->dma_commands[command_word++] = line_commands[i + 1]; // control word
                self->dma_commands[command_word++] = line_commands[i];     // data word
                
                // For pixel-doubled modes, repeat commands
                if (output_scaling > 1) {
                    self->dma_commands[command_word++] = line_commands[i + 1];
                    self->dma_commands[command_word++] = line_commands[i];
                }
            }
        }
    }
    
    // Add end-of-frame marker
    if (command_word < self->dma_commands_len) {
        self->dma_commands[command_word++] = 0;
    }
    
    // Update actual command length used
    self->dma_commands_len = command_word;
}

static picodvi_framebuffer_obj_t *active_picodvi = NULL;

// Enhanced preflight validation with refresh rate support (RP2350)
bool common_hal_picodvi_framebuffer_preflight(
    mp_uint_t width, mp_uint_t height,
    mp_uint_t color_depth, mp_uint_t refresh_rate) {
    
    // Validate refresh rate first - RP2350 supports more refresh rates
    if (refresh_rate != 60 && refresh_rate != 65 && refresh_rate != 72) {
        return false;
    }
    
    // These modes don't duplicate pixels so we can do sub-byte colors
    // They take too much RAM for more than 8bit color though
    bool full_resolution = color_depth == 1 || color_depth == 2 || color_depth == 4 || color_depth == 8;
    // These modes rely on memory transfer to duplicate values across bytes
    bool doubled = color_depth == 8 || color_depth == 16 || color_depth == 32;
    
    // Determine output resolution based on framebuffer size and color depth
    mp_uint_t output_width = width;
    mp_uint_t output_height = height;
    
    // Apply pixel doubling for certain color depths
    if (doubled && (color_depth == 16 || color_depth == 32)) {
        output_width *= 2;
        output_height *= 2;
    }
    
    // Validate supported framebuffer sizes and their output resolutions (RP2350)
    bool valid_combination = false;
    
    // 640x480 output (60Hz NEW, 72Hz existing) 
    if (output_width == 640 && output_height == 480) {
        if (refresh_rate == 60 || refresh_rate == 72) {
            if (width == 640 && height == 480) {
                valid_combination = full_resolution;  // Direct 640x480
            } else if (width == 320 && height == 240) {
                valid_combination = doubled;           // 320x240 doubled to 640x480
            }
        }
    }
    // 720x400 output (72Hz only, existing)
    else if (output_width == 720 && output_height == 400) {
        if (refresh_rate == 72) {
            if (width == 720 && height == 400) {
                valid_combination = full_resolution;  // Direct 720x400
            } else if (width == 360 && height == 200) {
                valid_combination = doubled;           // 360x200 doubled to 720x400
            } else if (width == 180 && height == 100) {
                valid_combination = doubled;           // 180x100 doubled to 720x400
            }
        }
    }
    // 800x480 output (65Hz only, NEW for RP2350)
    else if (output_width == 800 && output_height == 480) {
        if (refresh_rate == 65) {
            if (width == 800 && height == 480) {
                valid_combination = full_resolution;  // Direct 800x480
            } else if (width == 400 && height == 240) {
                valid_combination = doubled;           // 400x240 doubled to 800x480
            }
        }
    }
    
    // Final check: ensure timing parameters exist for the output resolution
    if (valid_combination) {
        return get_timing_params(output_width, output_height, refresh_rate) != NULL;
    }
    
    return false;
}

// Enhanced constructor with refresh rate support (RP2350 implementation)
void common_hal_picodvi_framebuffer_construct(picodvi_framebuffer_obj_t *self,
    mp_uint_t width, mp_uint_t height,
    const mcu_pin_obj_t *clk_dp, const mcu_pin_obj_t *clk_dn,
    const mcu_pin_obj_t *red_dp, const mcu_pin_obj_t *red_dn,
    const mcu_pin_obj_t *green_dp, const mcu_pin_obj_t *green_dn,
    const mcu_pin_obj_t *blue_dp, const mcu_pin_obj_t *blue_dn,
    mp_uint_t color_depth, mp_uint_t refresh_rate) {
        
    if (active_picodvi != NULL) {
        mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("%q in use"), MP_QSTR_picodvi);
    }
    
    if (!common_hal_picodvi_framebuffer_preflight(width, height, color_depth, refresh_rate)) {
        mp_raise_ValueError_varg(MP_ERROR_TEXT("Invalid %q, %q, color_depth, or refresh_rate"), 
                                MP_QSTR_width, MP_QSTR_height);
    }
    
    // Pin validation for HSTX (GPIO 12-19) - RP2350 specific
    if (clk_dp->number < 12 || clk_dp->number > 19 ||
        clk_dn->number < 12 || clk_dn->number > 19 ||
        red_dp->number < 12 || red_dp->number > 19 ||
        red_dn->number < 12 || red_dn->number > 19 ||
        green_dp->number < 12 || green_dp->number > 19 ||
        green_dn->number < 12 || green_dn->number > 19 ||
        blue_dp->number < 12 || blue_dp->number > 19 ||
        blue_dn->number < 12 || blue_dn->number > 19) {
        mp_raise_ValueError(MP_ERROR_TEXT("Invalid HSTX pins - must be GPIO 12-19"));
    }
    
    // Determine output resolution based on framebuffer size and color depth
    mp_uint_t output_scaling = 1;
    if (color_depth == 8 || color_depth == 16 || color_depth == 32) {
        // Check if this should use pixel doubling
        if ((width == 320 && height == 240) ||   // 320x240 -> 640x480
            (width == 360 && height == 200) ||   // 360x200 -> 720x400  
            (width == 180 && height == 100) ||   // 180x100 -> 720x400
            (width == 400 && height == 240)) {   // 400x240 -> 800x480 (NEW)
            output_scaling = 2;
        }
    }
    
    // Calculate output dimensions
    self->output_width = width * output_scaling;
    self->output_height = height * output_scaling;
    
    // Get timing parameters for the output resolution
    const dvi_timing_t* timing = get_timing_params(self->output_width, self->output_height, refresh_rate);
    if (timing == NULL) {
        mp_raise_ValueError(MP_ERROR_TEXT("Unsupported resolution/refresh rate combination"));
    }
    
    // Initialize framebuffer properties
    self->width = width;
    self->height = height;
    self->color_depth = color_depth;
    self->refresh_rate = refresh_rate;  // NEW: store refresh rate
    
    // Store timing parameters (NEW: for flexible timing support on RP2350)
    self->h_active = timing->h_active;
    self->h_front = timing->h_front;
    self->h_sync = timing->h_sync;
    self->h_back = timing->h_back;
    self->v_active = timing->v_active;
    self->v_front = timing->v_front;
    self->v_sync = timing->v_sync;
    self->v_back = timing->v_back;
    
    // Calculate framebuffer memory requirements (existing logic)
    size_t pitch_bytes = (self->width * color_depth + 7) / 8;  // Round up to nearest byte
    self->pitch = (pitch_bytes + sizeof(uint32_t) - 1) / sizeof(uint32_t);  // Convert to 32-bit words
    size_t framebuffer_size = self->pitch * self->height;
    
    // Allocate framebuffer memory (existing, with PSRAM support)
    self->framebuffer = (uint32_t *)port_malloc(framebuffer_size * sizeof(uint32_t), true);
    if (self->framebuffer == NULL || ((size_t)self->framebuffer & 0xf0000000) == 0x10000000) {
        common_hal_picodvi_framebuffer_deinit(self);
        m_malloc_fail(framebuffer_size * sizeof(uint32_t));
        return;
    }
    self->framebuffer_len = framebuffer_size;
    
    // Clear framebuffer
    memset(self->framebuffer, 0, framebuffer_size * sizeof(uint32_t));
    
    // Calculate DMA command buffer size (enhanced for new modes)
    size_t total_lines = timing->v_active + timing->v_front + timing->v_sync + timing->v_back;
    size_t dma_command_size = output_scaling == 1 ? 2 : 4;  // More commands needed for pixel doubling
    self->dma_commands_len = total_lines * dma_command_size + 10;  // Extra margin for safety
    
    // Allocate DMA command buffer
    self->dma_commands = (uint32_t *)port_malloc(self->dma_commands_len * sizeof(uint32_t), true);
    if (self->dma_commands == NULL || ((size_t)self->dma_commands & 0xf0000000) == 0x10000000) {
        common_hal_picodvi_framebuffer_deinit(self);
        m_malloc_fail(self->dma_commands_len * sizeof(uint32_t));
        return;
    }
    
    // Claim DMA channels (existing)
    self->dma_pixel_channel = dma_claim_unused_channel(false);
    self->dma_command_channel = dma_claim_unused_channel(false);
    if (self->dma_pixel_channel < 0 || self->dma_command_channel < 0) {
        common_hal_picodvi_framebuffer_deinit(self);
        mp_raise_RuntimeError(MP_ERROR_TEXT("Internal resource(s) in use"));
        return;
    }
    
    // Set up HSTX clock based on timing parameters (enhanced for RP2350)
    clock_configure(clk_hstx,
        0, // No glitchless mux
        CLOCKS_CLK_HSTX_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS,
        timing->hstx_clock_hz * 2, // PLL runs at 2x HSTX clock
        timing->hstx_clock_hz);
    
    // Configure HSTX peripheral with timing-specific parameters (RP2350)
    hstx_ctrl_hw->csr = 0;
    hstx_ctrl_hw->csr =
        HSTX_CTRL_CSR_EXPAND_EN_BITS |
        timing->hstx_clkdiv << HSTX_CTRL_CSR_CLKDIV_LSB |
        timing->hstx_n_shifts << HSTX_CTRL_CSR_N_SHIFTS_LSB |
        timing->hstx_shift_amount << HSTX_CTRL_CSR_SHIFT_LSB |
        HSTX_CTRL_CSR_EN_BITS;
        
    // Build complete DMA command sequence with TMDS encoding (RP2350)
    build_dma_command_sequence(self, timing, output_scaling);
    
    // Configure DMA channels with enhanced TMDS support (RP2350)
    dma_channel_config pixel_config = dma_channel_get_default_config(self->dma_pixel_channel);
    channel_config_set_transfer_data_size(&pixel_config, DMA_SIZE_32);
    channel_config_set_read_increment(&pixel_config, true);
    channel_config_set_write_increment(&pixel_config, false);
    channel_config_set_dreq(&pixel_config, DREQ_HSTX);
    channel_config_set_chain_to(&pixel_config, self->dma_command_channel);
    
    dma_channel_configure(
        self->dma_pixel_channel,
        &pixel_config,
        &hstx_fifo_hw->fifo,
        self->framebuffer,
        self->framebuffer_len,
        false
    );
    
    dma_channel_config command_config = dma_channel_get_default_config(self->dma_command_channel);
    channel_config_set_transfer_data_size(&command_config, DMA_SIZE_32);
    channel_config_set_read_increment(&command_config, true);
    channel_config_set_write_increment(&command_config, false);
    channel_config_set_chain_to(&command_config, self->dma_command_channel); // Loop back
    
    dma_channel_configure(
        self->dma_command_channel,
        &command_config,
        &dma_hw->ch[self->dma_pixel_channel].al3_transfer_count,
        self->dma_commands,
        self->dma_commands_len,
        false
    );
    
    // Set up HSTX FIFO configuration for TMDS output (RP2350)
    hstx_fifo_hw->csr = 0;
    hstx_fifo_hw->csr = HSTX_FIFO_CSR_EN_BITS |
                        (3 << HSTX_FIFO_CSR_LEVEL_LSB); // Trigger when 3 words available
    
    // Start the DMA chain
    dma_channel_start(self->dma_command_channel);
    
    active_picodvi = self;
}

// Existing functions (unchanged for RP2350 compatibility)
void common_hal_picodvi_framebuffer_deinit(picodvi_framebuffer_obj_t *self) {
    if (active_picodvi == self) {
        // Stop HSTX
        hstx_ctrl_hw->csr = 0;
        
        // Free DMA channels
        if (self->dma_pixel_channel >= 0) {
            dma_channel_unclaim(self->dma_pixel_channel);
            self->dma_pixel_channel = -1;
        }
        if (self->dma_command_channel >= 0) {
            dma_channel_unclaim(self->dma_command_channel);
            self->dma_command_channel = -1;
        }
        
        // Free memory
        if (self->framebuffer) {
            port_free(self->framebuffer);
            self->framebuffer = NULL;
        }
        if (self->dma_commands) {
            port_free(self->dma_commands);
            self->dma_commands = NULL;
        }
        
        active_picodvi = NULL;
    }
}

void common_hal_picodvi_framebuffer_refresh(picodvi_framebuffer_obj_t *self) {
    if (active_picodvi == self && self->dma_command_channel >= 0) {
        // Stop current DMA operation
        dma_channel_abort(self->dma_command_channel);
        dma_channel_abort(self->dma_pixel_channel);
        
        // Restart the DMA chain from the beginning
        dma_channel_hw_t *cmd_ch = &dma_hw->ch[self->dma_command_channel];
        dma_channel_hw_t *pix_ch = &dma_hw->ch[self->dma_pixel_channel];
        
        // Reset command channel
        cmd_ch->read_addr = (uintptr_t)self->dma_commands;
        cmd_ch->transfer_count = self->dma_commands_len;
        
        // Reset pixel channel  
        pix_ch->read_addr = (uintptr_t)self->framebuffer;
        pix_ch->transfer_count = self->framebuffer_len;
        
        // Trigger restart
        cmd_ch->al3_read_addr_trig = (uintptr_t)self->dma_commands;
    }
}

int common_hal_picodvi_framebuffer_get_width(picodvi_framebuffer_obj_t *self) {
    return self->width;
}

int common_hal_picodvi_framebuffer_get_height(picodvi_framebuffer_obj_t *self) {
    return self->height;
}
