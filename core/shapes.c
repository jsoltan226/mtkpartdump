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
#include "shapes.h"
#include "math.h"
#include <stdlib.h>

void rect_clip(rect_t *r, const rect_t *max)
{
    if (r == NULL || max == NULL) return;

    vec2d_t a1, b1;

    a1.x = u_max(r->x, max->x);
    a1.y = u_max(r->y, max->y);
    b1.x = u_min(r->x + r->w, max->x + max->w);
    b1.y = u_min(r->y + r->h, max->y + max->h);

    r->x = a1.x;
    r->y = a1.y;
    r->w = u_max(0, b1.x - a1.x);
    r->h = u_max(0, b1.y - a1.y);
}
