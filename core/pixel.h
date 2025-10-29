#ifndef PIXEL_H_
#define PIXEL_H_
#include "static-tests.h"

#include "shapes.h"
#include "int.h"

typedef color_RGBA32_t pixel_t;

typedef enum pixelfmt {
    RGB24,  /* 8-bit Red, 8-bit Green, 8-bit Blue - total bits: 24 */
    BGR24,  /* 8-bit Blue, 8-bit Green, 8-bit Red - total bits: 24 */
    RGBA32, /* 8-bit Red, Green, Blue and Alpha - total bits: 32 */
    BGRA32, /* 8-bit Blue, Green, Red and Alpha - total bits: 32 */

    /* Different names for the same thing */
    RGB888 = RGB24,
    BGR888 = BGR24,
    RGBA8888 = RGBA32,
    BGRA8888 = BGRA32,

    /* These are the exact same as the above RGBA and BGRA,
     * except that the `A` (Alpha) channel is ignored/unused.
     * Typically used by raw displays. */
    RGBX32,
    RGBX8888 = RGBX32,
    BGRX32,
    BGRX888 = BGRX32,
} pixelfmt_t;

struct pixel_flat_data {
    pixel_t *buf;
    u32 w, h;
};

struct pixel_row_data {
    pixel_t **rows;
    u32 w, h;
};

#define EMPTY_PIXEL ((pixel_t) { 0 })
#define BLACK_PIXEL ((pixel_t) { 0, 0, 0, 255 })
#define WHITE_PIXEL ((pixel_t) { 255, 255, 255, 255 })

i32 pixel_row_data_init(struct pixel_row_data *out, u32 w, u32 h);
void pixel_row_data_destroy(struct pixel_row_data *data);

void pixel_data_row2flat(struct pixel_row_data *in, struct pixel_flat_data *out);
void pixel_data_flat2row(struct pixel_flat_data *in, struct pixel_row_data *out);

#endif /* PIXEL_H_ */
