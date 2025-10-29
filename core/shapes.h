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
