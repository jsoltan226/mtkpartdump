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
