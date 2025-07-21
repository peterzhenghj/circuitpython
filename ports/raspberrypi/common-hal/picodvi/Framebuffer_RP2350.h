// =============================================================================
// 1. ports/raspberrypi/common-hal/picodvi/Framebuffer_RP2350.h (Enhanced)
// =============================================================================
#pragma once

#include "py/obj.h"

// RP2350-specific framebuffer structure with enhanced capabilities
typedef struct {
    mp_obj_base_t base;
    
    // Display parameters
    uint16_t width;
    uint16_t height;
    uint8_t color_depth;
    uint8_t refresh_rate;        // NEW: refresh rate field for RP2350
    
    // Output resolution (may differ from framebuffer size due to doubling)
    uint16_t output_width;
    uint16_t output_height;
    
    // Timing parameters (NEW: for flexible timing support on RP2350)
    uint16_t h_active, h_front, h_sync, h_back;
    uint16_t v_active, v_front, v_sync, v_back;
    
    // Hardware resources (existing)
    int dma_pixel_channel;
    int dma_command_channel;
    
    // Memory (existing)
    uint32_t *framebuffer;
    size_t framebuffer_len;
    uint32_t pitch;             // 32-bit words per line
    
    // DMA commands (existing)
    uint32_t *dma_commands;
    size_t dma_commands_len;
    
} picodvi_framebuffer_obj_t;

// RP2350-specific function prototypes with enhanced API
void common_hal_picodvi_framebuffer_construct(picodvi_framebuffer_obj_t *self,
    mp_uint_t width, mp_uint_t height,
    const mcu_pin_obj_t *clk_dp, const mcu_pin_obj_t *clk_dn,
    const mcu_pin_obj_t *red_dp, const mcu_pin_obj_t *red_dn,
    const mcu_pin_obj_t *green_dp, const mcu_pin_obj_t *green_dn,
    const mcu_pin_obj_t *blue_dp, const mcu_pin_obj_t *blue_dn,
    mp_uint_t color_depth, mp_uint_t refresh_rate);

bool common_hal_picodvi_framebuffer_preflight(
    mp_uint_t width, mp_uint_t height,
    mp_uint_t color_depth, mp_uint_t refresh_rate);

void common_hal_picodvi_framebuffer_deinit(picodvi_framebuffer_obj_t *self);
void common_hal_picodvi_framebuffer_refresh(picodvi_framebuffer_obj_t *self);
int common_hal_picodvi_framebuffer_get_width(picodvi_framebuffer_obj_t *self);
int common_hal_picodvi_framebuffer_get_height(picodvi_framebuffer_obj_t *self);
