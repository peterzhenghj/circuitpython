// =============================================================================
// 3. ports/raspberrypi/bindings/picodvi/__init__.c (Compatible Enhancement)
// =============================================================================
/*
 * Enhanced picodvi module initialization (compatible with both RP2040 and RP2350)
 * 
 * This adds refresh rate constants that are available on both platforms,
 * though RP2040 may not support all refresh rates for all resolutions.
 * The preflight validation in each platform's implementation will handle
 * the specific limitations.
 */

#include "py/obj.h"
#include "py/runtime.h"
#include "bindings/picodvi/Framebuffer.h"

//| """DVI video output using PicoDVI
//|
//| .. note:: This module requires specific pins. On RP2040, PIO pins are used.
//|           On RP2350, HSTX pins (GPIO 12-19) are required.
//|
//| """

STATIC const mp_rom_map_elem_t picodvi_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_picodvi) },
    { MP_ROM_QSTR(MP_QSTR_Framebuffer), MP_ROM_PTR(&picodvi_framebuffer_type) },
    
    // Refresh rate constants (compatible with both RP2040 and RP2350)
    // NOTE: Not all refresh rates may be supported on all platforms
    // Use preflight validation to check compatibility
    { MP_ROM_QSTR(MP_QSTR_REFRESH_60HZ), MP_ROM_INT(60) },
    { MP_ROM_QSTR(MP_QSTR_REFRESH_65HZ), MP_ROM_INT(65) },
    { MP_ROM_QSTR(MP_QSTR_REFRESH_72HZ), MP_ROM_INT(72) },
};

STATIC MP_DEFINE_CONST_DICT(picodvi_module_globals, picodvi_module_globals_table);

const mp_obj_module_t picodvi_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&picodvi_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_picodvi, picodvi_module);