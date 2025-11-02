/* mtkpartdump - Mediatek partition dump tool
 * Copyright (C) 2025 Jan Sołtan <jsoltan226@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>
*/
#ifndef U_SHAPES_H_
#define U_SHAPES_H_
#include "static-tests.h"

#include "int.h"
#include <assert.h>

typedef struct {
    f32 x, y;
} vec2d_t;

typedef struct {
    f32 x, y, z;
} vec3d_t;

typedef struct {
    i32 x, y;
    u32 w, h;
} rect_t;

typedef struct {
    u8 r, g, b, a;
} color_RGBA32_t;

static_assert(sizeof(color_RGBA32_t) == 4,
    "The size of color_RGBA32_t must be 4 bytes (32 bits)");

#define rect_arg_expand(rect) (rect).x, (rect).y, (rect).w, (rect).h
#define rectp_arg_expand(rect) (rect)->x, (rect)->y, (rect)->w, (rect)->h

void rect_clip(rect_t *r, const rect_t *max);

#endif /* U_SHAPES_H_ */
