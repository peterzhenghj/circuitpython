/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * SPDX-FileCopyrightText: Copyright (c) 2013-2016 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "py/obj.h"
#include "py/mpstate.h"
// CIRCUITPY-CHANGE
#include "shared/runtime/interrupt_char.h"

#if MICROPY_KBD_EXCEPTION

#ifdef __ZEPHYR__
#include <zephyr/kernel.h>

// This semaphore is released when an interrupt character is seen. Core CP code
// can wait for this release but shouldn't take it. They should return instead
// after cancelling what they were doing.
K_SEM_DEFINE(mp_interrupt_sem, 0, 1);
#endif

int mp_interrupt_char = -1;

void mp_hal_set_interrupt_char(int c) {
    mp_interrupt_char = c;
}

// CIRCUITPY-CHANGE
// Check to see if we've been CTRL-C'ed by autoreload or the user.
bool mp_hal_is_interrupted(void) {
    return MP_STATE_THREAD(mp_pending_exception) != MP_OBJ_FROM_PTR(NULL);
}

#endif
