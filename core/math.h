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
#ifndef CORE_MATH_H_
#define CORE_MATH_H_
#include "static-tests.h"

#include "int.h"
#include "pixel.h"
#include "shapes.h"
#include <stdbool.h>

#define u_min(a, b) (a < b ? a : b)
#define u_max(a, b) (a > b ? a : b)
#define u_clamp(x, min, max) (u_min(u_max(x, min), max))

/* The simplest collision checking implementation;
 * returns true if 2 rectangles overlap
 *
 * Note that the rects' `w` and `h` fields must be small enough
 * for an `int` (`i32`) to not overflow.
 */
static inline bool u_collision(
    const rect_t *r1,
    const rect_t *r2
)
{
    return (
        r1->x <= r2->x + (i32)r2->w &&
        r1->x + (i32)r1->w >= r2->x &&
        r1->y <= r2->y + (i32)r2->h &&
        r1->y + (i32)r1->h >= r2->y
    );
}

/* Converts a fixed point 16.16 number to float 32 */
static inline f32 u_fp1616_to_f32(const i32 num)
{
    f32 ret = (f32)num;
    ret = ret / (f32)(1 << 16);
    return ret;
}

static inline void u_rect_from_pixel_data(
    const struct pixel_flat_data *data,
    rect_t *o
)
{
    o->x = 0;
    o->y = 0;
    o->w = data->w;
    o->h = data->h;
}

#endif /* CORE_MATH_H_ */
