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
#include "mtkpartdump.h"
#include "mtkparthdr.h"
#include "arg.h"
#include <core/log.h>
#include <core/util.h>
#include <core/math.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

#define MODULE_NAME "mtkpartdump"

static_assert(MTK_PART_NAME_LEN == 32,
    "This code expects MTK_PART_NAME_LEN to be 32");

static void print_part_header(const struct mtk_partition_header_data *hdr,
    u32 hdr_index);
static void print_ext_part_header(const struct mtk_part_header_extension *ext);

static i32 do_save_header(union mtk_partition_header *hdr, u32 hdr_index);
static i32 do_extract_part(FILE *in_fp, size_t n_bytes, const char *out_path);

static u32 get_aligned_part_size(const struct mtk_partition_header_data *hdr);
static u64 get_full_part_size(const struct mtk_partition_header_data *hdr);
static u64 get_full_aligned_part_size(
    const struct mtk_partition_header_data *hdr
);
static void * get_full_memory_address(
    const struct mtk_partition_header_data *hdr
);
static const char *get_img_type_string(u32 img_type);

static char * get_out_filename_from_part_name(
    const char part_name[MTK_PART_NAME_LEN],
    bool is_header, u32 index
);

void mtkpart_dump_file(FILE *fp, u32 flags)
{
    bool chain = flags & ARG_FLAG_CHAIN;

    s_log_debug("chain: %d, save: %d, extract: %d",
        (flags & ARG_FLAG_CHAIN) || 0,
        (flags & ARG_FLAG_SAVE_HDR) || 0,
        (flags & ARG_FLAG_EXTRACT_PART) || 0
    );

    u32 index = 0;

    union mtk_partition_header hdr = { 0 };
    do {
        s_log_verbose("Processing header no. %u...", index);

        i32 ret = fread(&hdr.buf_, 1, MTK_PART_HEADER_SIZE, fp);
        if (ret != MTK_PART_HEADER_SIZE && ferror(fp)) {
            s_log_error("Failed to fread() the header intro: %s",
                strerror(errno));
            return;
        } else if (ret != MTK_PART_HEADER_SIZE && feof(fp)) {
            s_log_error("File is too small (end of file reached)");
            return;
        } else if (ret != MTK_PART_HEADER_SIZE) {
            s_log_error("Read an incorrect number of bytes from the input file "
                "(read: %#x, expected: %#x)", ret, MTK_PART_HEADER_SIZE);
            return;
        }

        if (hdr.data.magic != MTK_PART_MAGIC) {
            s_log_error("Invalid magic: 0x%.8x (expected: 0x%.8x)",
                hdr.data.magic, MTK_PART_MAGIC);
            return;
        }

        print_part_header(&hdr.data, index);

        if (flags & ARG_FLAG_SAVE_HDR) {
            if (do_save_header(&hdr, index)) {
                s_log_error("Failed to save the partition header!");
                /* A failure here doesn't really impact anything
                 * further down the line */
            }
        }

        const u64 full_part_size = get_full_aligned_part_size(&hdr.data);
        if (flags & ARG_FLAG_EXTRACT_PART) {

            char *out_path = get_out_filename_from_part_name(
                hdr.data.part_name, false, index
            );

            i32 ret = do_extract_part(fp, full_part_size, out_path);

            u_nfree(&out_path);

            if (ret) {
                s_log_error("Failed to extract the partition contents "
                    "from \"%.32s\". Terminating chain uncoditionally!",
                    hdr.data.part_name);
                chain = false;
            }

        /* If we aren't extracting the content of the partition,
         * just advance past it */
        } else if (chain && fseek(fp, full_part_size, SEEK_CUR)) {
            s_log_error("Failed to seek to the next header in the chain "
                "(%llu bytes forward): %s. "
                "Terminating chain uncoditionally!",
                full_part_size, strerror(errno)
            );
            chain = false;
        }

        if (chain && hdr.data.ext.magic != MTK_PART_EXT_MAGIC) {
            s_log_verbose("ext magic mismatch: 0x%.8x (expected 0x%.8x); "
                "terminating chain uncoditionally",
                hdr.data.ext.magic, MTK_PART_EXT_MAGIC);
            chain = false;
        } else if (chain && hdr.data.ext.is_image_list_end) {
            s_log_verbose("End of chain reached");
            chain = false;
        }

        index++;
    } while (chain);
}

#define log_magic(prepend_str, magic) s_log_info(                   \
        "%s0x%.8x, // (BE: 0x%.2x%.2x%.2x%.2x)", prepend_str, magic,\
        (u8)((magic & 0x000000FF) >> 0), /* convert endianness */   \
        (u8)((magic & 0x0000FF00) >> 8),                            \
        (u8)((magic & 0x00FF0000) >> 16),                           \
        (u8)((magic & 0xFF000000) >> 24)                            \
    )

static void print_part_header(const struct mtk_partition_header_data *hdr,
    u32 hdr_index)
{
    const char *old_line_verbose = NULL, *old_line_info = NULL;
    s_configure_log_line(S_LOG_VERBOSE, "%s\n", &old_line_verbose);
    s_configure_log_line(S_LOG_INFO, "%s\n", &old_line_info);

    char *hdr_name =
        get_out_filename_from_part_name(hdr->part_name, true, hdr_index);

    s_log_verbose("===== Begin Mediatek partition header dump =====");
    s_log_info("union mtk_partition_header %s = {", hdr_name);
    s_log_info("    .data = {");
    log_magic ("        .magic = ", hdr->magic);
    s_log_info("        .part_size = %#x, "
        "// aligned: %#x, full: %#x, aligned full: %#x",
        hdr->part_size,
        get_aligned_part_size(hdr),
        get_full_part_size(hdr),
        get_full_aligned_part_size(hdr)
    );
    s_log_info("        .part_name = \"%.32s\",", hdr->part_name);
    s_log_info("        .memory_address = %p, // full: %p",
        hdr->memory_address, get_full_memory_address(hdr));
    s_log_info("        .memory_address_mode = %#x,", hdr->memory_address_mode);
    s_log_info("        .ext = {");
    print_ext_part_header(&hdr->ext);
    s_log_info("        }");
    s_log_info("    }");
    s_log_info("};");
    s_log_verbose("=====  End Mediatek partition header dump  =====");

    u_nfree(&hdr_name);

    s_configure_log_line(S_LOG_VERBOSE, old_line_verbose, NULL);
    s_configure_log_line(S_LOG_INFO, old_line_info, NULL);
}

static void print_ext_part_header(const struct mtk_part_header_extension *ext)
{
    const char *old_line_verbose = NULL, *old_line_info = NULL;
    s_configure_log_line(S_LOG_VERBOSE, "%s\n", &old_line_verbose);
    s_configure_log_line(S_LOG_INFO, "%s\n", &old_line_info);

    log_magic ("            .magic = ", ext->magic);
    if (ext->magic != MTK_PART_EXT_MAGIC) {
        s_log_info("            // magic mismatch (expected 0x%.8x); "
            "skipping the rest of the extended header",
            MTK_PART_EXT_MAGIC);
        return;
    }
    s_log_info("            .hdr_size = %#x,", ext->hdr_size);
    s_log_info("            .hdr_version = %#x,", ext->hdr_version);
    s_log_info("            .img_type = %#x, // (%s)", ext->img_type,
            get_img_type_string(ext->img_type));
    s_log_info("            .is_image_list_end = %#x,",
            ext->is_image_list_end);
    s_log_info("            .size_alignment_bytes = %#x,",
            ext->size_alignment_bytes);
    s_log_info("            .part_size_hi = %#x,", ext->part_size_hi);
    s_log_info("            .memory_address_hi = %#x",
            ext->memory_address_hi);

    s_configure_log_line(S_LOG_VERBOSE, old_line_verbose, NULL);
    s_configure_log_line(S_LOG_INFO, old_line_info, NULL);
}

static u32 get_aligned_part_size(const struct mtk_partition_header_data *hdr)
{
    if (hdr->ext.magic == MTK_PART_EXT_MAGIC &&
        hdr->ext.size_alignment_bytes != 0)
    {
        /* Round up to the next multiple of `align` */
        const u32 size = hdr->part_size;
        const u32 align = hdr->ext.size_alignment_bytes;
        return ((size + align - 1) / align) * align;
    } else {
        return hdr->part_size;
    }
}

static u64 get_full_part_size(const struct mtk_partition_header_data *hdr)
{
    if (hdr->ext.magic == MTK_PART_EXT_MAGIC) {
        const u64 high = (u64)hdr->ext.part_size_hi << 32;
        return high | hdr->part_size;
    } else {
        return (u64)hdr->part_size;
    }
}

static u64 get_full_aligned_part_size(
    const struct mtk_partition_header_data *hdr
)
{
    const u32 low = get_aligned_part_size(hdr);
    const u64 high = get_full_part_size(hdr) & 0xFFFFFFFF00000000ULL;

    return high | low;
}

static void * get_full_memory_address(
    const struct mtk_partition_header_data *hdr
)
{
    if (hdr->ext.magic == MTK_PART_EXT_MAGIC) {
        const u64 high = (u64)hdr->ext.memory_address_hi << 32;
        return (void *)(high | hdr->memory_address);
    } else {
        return (void *)((u64)hdr->memory_address);
    }
}

static const char *get_img_type_string(u32 img_type)
{
    switch (img_type) {
#define X_(name, value)                 \
        case value:                     \
            return "IMG_TYPE_" #name;   \

        MTK_PART_EXT_IMG_TYPE_LIST
#undef X_
        default:
            return "N/A";
    }
}

static char * get_out_filename_from_part_name(
    const char part_name[MTK_PART_NAME_LEN],
    bool is_header, u32 index
)
{
    i32 size = snprintf(NULL, 0, "%.32s.%s_0x%x.bin",
        part_name,
        is_header ? "header" : "extracted",
        index
    );
    s_assert(size >= 0, "snprintf failed!");

    char *buf = malloc(size + 1 /* For the '\0' terminator */);
    s_assert(buf != NULL, "malloc failed for new filename buffer");

    i32 ret = snprintf(buf, size + 1, "%.32s.%s_0x%x.bin",
        part_name,
        is_header ? "header" : "extracted",
        index
    );
    s_assert(ret == size, "snprintf failed (ret: %d, expected: %d)", ret, size);

    return buf;
}

static i32 do_save_header(union mtk_partition_header *hdr, u32 index)
{
    char *out_path_str = NULL;
    FILE *out_fp = NULL;

    out_path_str =
        get_out_filename_from_part_name(hdr->data.part_name, true, index);
    s_log_verbose("Saving partition header to file \"%s\"...", out_path_str);

    out_fp = fopen(out_path_str, "wb");
    if (out_fp == NULL) {
        goto_error("Failed to open file \"%s\" for writing: %s",
            out_path_str, strerror(errno));
    }

    i32 ret = fwrite(hdr, sizeof(union mtk_partition_header), 1, out_fp);
    if (ret != 1) {
        goto_error("Failed to write the partiton header to file \"%s\": %s",
            out_path_str, strerror(errno));
    }

    if (fclose(out_fp)) {
        out_fp = NULL;
        goto_error("Failed to close the output file \"%s\": %s",
            out_path_str, strerror(errno));
    }
    out_fp = NULL;

    u_nfree(&out_path_str);

    return 0;

err:
    if (out_fp != NULL) {
        if (fclose(out_fp)) {
            s_log_error("Failed to close the output file \"%s\": %s",
                (out_path_str ? out_path_str : "<n/a>"), strerror(errno));
        }
        out_fp = NULL;
    }
    if (out_path_str != NULL)
        u_nfree(&out_path_str);

    return 1;
}

static i32 do_extract_part(FILE *in_fp, size_t n_bytes, const char *out_path)
{
    FILE *out_fp = NULL;
    u8 *buf = NULL;

    s_log_verbose("Extracting partition content to file \"%s\"...", out_path);

    out_fp = fopen(out_path, "wb");
    if (out_fp == NULL) {
        goto_error("Failed to open output file \"%s\": %s. ",
            out_path, strerror(errno));
    }

    /* Copy the contents in 1MB blocks to reduce syscall overhead.
     * The OS should handle further buffering (e.g. down to disk block size)
     * by itself. */
#define BLOCK_BUF_SIZE (1024 * 1024)

    /* We might not need the full 1MB if the partition is small enough */
    const size_t buf_size = u_min(BLOCK_BUF_SIZE, n_bytes);

    buf = malloc(buf_size);
    if (buf == NULL) {
        goto_error("Failed to allocate %llu bytes for the copy buffer",
            buf_size);
    }

    size_t n_bytes_left = n_bytes;
    while (n_bytes_left > 0) {
        const size_t chunk = u_min(buf_size, n_bytes_left);

        size_t n_read = fread(buf, 1, chunk, in_fp);
        if (n_read != chunk) {
            if (feof(in_fp)) {
                goto_error("Input file doesn't contain the full partition "
                    "content (unexpected end of file while reading)!");
            } else if (ferror(in_fp)) {
                goto_error("Unexpected error while reading from input file: %s",
                    strerror(errno));
            }
        }

        size_t n_written = fwrite(buf, 1, chunk, out_fp);
        if (n_written != chunk)
            goto_error("Failed to write to output file: %s", strerror(errno));

        n_bytes_left -= chunk;
    }

    if (fclose(out_fp)) {
        out_fp = NULL;
        goto_error("Failed to close the output file \"%s\": %s",
            out_path, strerror(errno));
    }
    out_fp = NULL;

    return 0;

err:
    if (buf != NULL) {
        u_nfree(&buf);
    }
    if (out_fp != NULL) {
        if (fclose(out_fp))
            s_log_error("Failed to close the output file \"%s\": %s",
                out_path, strerror(errno));
        out_fp = NULL;
    }
    return 1;
}
