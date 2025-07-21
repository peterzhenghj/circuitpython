// =============================================================================
// 3. ports/raspberrypi/bindings/picodvi/__init__.c (Complete Implementation)
// =============================================================================
/*
 * Enhanced picodvi module bindings (compatible with both RP2040 and RP2350)
 * 
 * This file contains the complete Python API implementation with enhanced
 * refresh rate support. The refresh_rate parameter defaults to 72 to maintain
 * backward compatibility. Platform-specific validation in preflight functions
 * will determine what refresh rates are actually supported.
 */

#include "py/obj.h"
#include "py/runtime.h"
#include "py/objproperty.h"
#include "bindings/picodvi/Framebuffer.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/util.h"

//| """DVI video output using PicoDVI
//|
//| .. note:: This module requires specific pins. On RP2040, PIO pins are used.
//|           On RP2350, HSTX pins (GPIO 12-19) are required.
//|
//| """

//| class Framebuffer:
//|     """A picodvi framebuffer."""
//|
//|     def __init__(
//|         self,
//|         width: int,
//|         height: int,
//|         *,
//|         clk_dp: microcontroller.Pin,
//|         clk_dn: microcontroller.Pin,
//|         red_dp: microcontroller.Pin,
//|         red_dn: microcontroller.Pin,
//|         green_dp: microcontroller.Pin,
//|         green_dn: microcontroller.Pin,
//|         blue_dp: microcontroller.Pin,
//|         blue_dn: microcontroller.Pin,
//|         color_depth: int = 8,
//|         refresh_rate: int = 72,
//|     ) -> None:
//|         """Create a Framebuffer object with the given parameters.
//|
//|         :param int width: the width of the framebuffer
//|         :param int height: the height of the framebuffer
//|         :param microcontroller.Pin clk_dp: the positive clock pin
//|         :param microcontroller.Pin clk_dn: the negative clock pin
//|         :param microcontroller.Pin red_dp: the positive red pin
//|         :param microcontroller.Pin red_dn: the negative red pin
//|         :param microcontroller.Pin green_dp: the positive green pin
//|         :param microcontroller.Pin green_dn: the negative green pin
//|         :param microcontroller.Pin blue_dp: the positive blue pin
//|         :param microcontroller.Pin blue_dn: the negative blue pin
//|         :param int color_depth: the color depth in bits per pixel
//|         :param int refresh_rate: the refresh rate in Hz (default 72, maintains backward compatibility)
//|
//|         Create a Framebuffer object with the given dimensions. Memory is allocated
//|         and then moved outside on VM end.
//|
//|         This will change the system clock speed to match the DVI signal. Make sure to
//|         initialize other objects after this one so they account for the changed clock.
//|
//|         This allocates a very large framebuffer and is most likely to succeed the
//|         earlier it is attempted.
//|         
//|         On RP2040, each dp and dn pair of pins must be neighboring, such as 19 and 20.
//|         They must also be ordered the same way. In other words, dp must be less than dn
//|         for all pairs or dp must be greater than dn for all pairs.
//|         
//|         On RP2350, all pins must be HSTX outputs (GPIO 12-19) but can be in any order.
//|         
//|         The framebuffer pixel format varies depending on color_depth:
//|         
//|         * 1 - Each bit is a pixel. Either white (1) or black (0).
//|         * 2 - Each two bits is a pixel. Grayscale between white (0x3) and black (0x0).
//|         * 4 - Each nibble is a pixel in RGB format. The fourth bit is ignored. (RP2350 only)
//|         * 8 - Each byte is a pixel in RGB332 format.
//|         * 16 - Each two bytes is a pixel in RGB565 format.
//|         * 32 - Each four bytes is a pixel in RGB888 format. The alpha byte is ignored. (RP2350 only)
//|         
//|         Monochrome modes (color_depth 1 and 2) must be full resolution. Other modes
//|         may be full resolution or pixel doubled to save memory.
//|         
//|         The refresh_rate parameter controls the DVI output timing. Not all refresh rates
//|         may be supported on all platforms. Use common_hal_picodvi_framebuffer_preflight
//|         to check compatibility.
//|         """
//|         ...

STATIC mp_obj_t picodvi_framebuffer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_width, ARG_height, ARG_clk_dp, ARG_clk_dn, ARG_red_dp, ARG_red_dn,
           ARG_green_dp, ARG_green_dn, ARG_blue_dp, ARG_blue_dn, ARG_color_depth, ARG_refresh_rate };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_width, MP_ARG_REQUIRED | MP_ARG_INT },
        { MP_QSTR_height, MP_ARG_REQUIRED | MP_ARG_INT },
        { MP_QSTR_clk_dp, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_clk_dn, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_red_dp, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_red_dn, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_green_dp, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_green_dn, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_blue_dp, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_blue_dn, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_color_depth, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 8} },
        { MP_QSTR_refresh_rate, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 72} }, // NEW parameter with default 72
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_int_t width = args[ARG_width].u_int;
    mp_int_t height = args[ARG_height].u_int;
    mp_int_t color_depth = args[ARG_color_depth].u_int;
    mp_int_t refresh_rate = args[ARG_refresh_rate].u_int; // NEW parameter

    const mcu_pin_obj_t *clk_dp = validate_obj_is_free_pin(args[ARG_clk_dp].u_obj, MP_QSTR_clk_dp);
    const mcu_pin_obj_t *clk_dn = validate_obj_is_free_pin(args[ARG_clk_dn].u_obj, MP_QSTR_clk_dn);
    const mcu_pin_obj_t *red_dp = validate_obj_is_free_pin(args[ARG_red_dp].u_obj, MP_QSTR_red_dp);
    const mcu_pin_obj_t *red_dn = validate_obj_is_free_pin(args[ARG_red_dn].u_obj, MP_QSTR_red_dn);
    const mcu_pin_obj_t *green_dp = validate_obj_is_free_pin(args[ARG_green_dp].u_obj, MP_QSTR_green_dp);
    const mcu_pin_obj_t *green_dn = validate_obj_is_free_pin(args[ARG_green_dn].u_obj, MP_QSTR_green_dn);
    const mcu_pin_obj_t *blue_dp = validate_obj_is_free_pin(args[ARG_blue_dp].u_obj, MP_QSTR_blue_dp);
    const mcu_pin_obj_t *blue_dn = validate_obj_is_free_pin(args[ARG_blue_dn].u_obj, MP_QSTR_blue_dn);

    // Preflight validation with platform-specific refresh rate support
    if (!common_hal_picodvi_framebuffer_preflight(width, height, color_depth, refresh_rate)) {
        mp_raise_ValueError_varg(MP_ERROR_TEXT("Invalid width (%d), height (%d), color_depth (%d) or refresh_rate (%d)"), width, height, color_depth, refresh_rate);
    }

    picodvi_framebuffer_obj_t *self = m_new_obj(picodvi_framebuffer_obj_t);
    self->base.type = &picodvi_framebuffer_type;

    // Call platform-specific constructor with refresh rate (enhanced for RP2350, backward compatible for RP2040)
    common_hal_picodvi_framebuffer_construct(
        self, width, height, clk_dp, clk_dn, red_dp, red_dn,
        green_dp, green_dn, blue_dp, blue_dn, color_depth, refresh_rate
    );

    return MP_OBJ_FROM_PTR(self);
}

//| def deinit(self) -> None:
//|     """Free the resources (pins, timers, etc.) associated with this
//|     picodvi.Framebuffer instance. After deinitialization, no further
//|     operations may be performed."""
//|     ...
STATIC mp_obj_t picodvi_framebuffer_deinit(mp_obj_t self_in) {
    picodvi_framebuffer_obj_t *self = (picodvi_framebuffer_obj_t *)self_in;
    common_hal_picodvi_framebuffer_deinit(self);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(picodvi_framebuffer_deinit_obj, picodvi_framebuffer_deinit);

//| def __enter__(self) -> Framebuffer:
//|     """No-op used by Context Managers."""
//|     ...
// Provided by context manager helper.

//| def __exit__(self) -> None:
//|     """Automatically deinitializes when exiting a context. See
//|     :ref:`lifetime-and-contextmanagers` for more info."""
//|     ...
STATIC mp_obj_t picodvi_framebuffer___exit__(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    common_hal_picodvi_framebuffer_deinit(args[0]);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(picodvi_framebuffer___exit___obj, 4, 4, picodvi_framebuffer___exit__);

//| width: int
//| """The width of the framebuffer, in pixels. It may be doubled for output."""
STATIC mp_obj_t picodvi_framebuffer_get_width(mp_obj_t self_in) {
    picodvi_framebuffer_obj_t *self = (picodvi_framebuffer_obj_t *)self_in;
    return MP_OBJ_NEW_SMALL_INT(common_hal_picodvi_framebuffer_get_width(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(picodvi_framebuffer_get_width_obj, picodvi_framebuffer_get_width);

MP_PROPERTY_GETTER(picodvi_framebuffer_width_obj,
    (mp_obj_t)&picodvi_framebuffer_get_width_obj);

//| height: int
//| """The height of the framebuffer, in pixels. It may be doubled for output."""
STATIC mp_obj_t picodvi_framebuffer_get_height(mp_obj_t self_in) {
    picodvi_framebuffer_obj_t *self = (picodvi_framebuffer_obj_t *)self_in;
    return MP_OBJ_NEW_SMALL_INT(common_hal_picodvi_framebuffer_get_height(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(picodvi_framebuffer_get_height_obj, picodvi_framebuffer_get_height);

MP_PROPERTY_GETTER(picodvi_framebuffer_height_obj,
    (mp_obj_t)&picodvi_framebuffer_get_height_obj);

STATIC const mp_rom_map_elem_t picodvi_framebuffer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&picodvi_framebuffer_deinit_obj) },

    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&default___enter___obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&picodvi_framebuffer___exit___obj) },

    { MP_ROM_QSTR(MP_QSTR_width), MP_ROM_PTR(&picodvi_framebuffer_width_obj) },
    { MP_ROM_QSTR(MP_QSTR_height), MP_ROM_PTR(&picodvi_framebuffer_height_obj) },
};
STATIC MP_DEFINE_CONST_DICT(picodvi_framebuffer_locals_dict, picodvi_framebuffer_locals_dict_table);

const mp_obj_type_t picodvi_framebuffer_type = {
    { &mp_type_type },
    .name = MP_QSTR_Framebuffer,
    .make_new = picodvi_framebuffer_make_new,
    .locals_dict = (mp_obj_dict_t *)&picodvi_framebuffer_locals_dict,
};

// Module constants and initialization
STATIC const mp_rom_map_elem_t picodvi_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_picodvi) },
    { MP_ROM_QSTR(MP_QSTR_Framebuffer), MP_ROM_PTR(&picodvi_framebuffer_type) },
    
    // Refresh rate constants (compatible with both RP2040 and RP2350)
    // Platform-specific validation will determine actual support
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
