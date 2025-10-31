#include "mtkparthdr.h"
#include <core/log.h>
#include <core/util.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define MODULE_NAME "mtkpartdump"

static i32 setup_log(void);

static void dump_part_header(struct mtk_partition_header_data *hdr);
static void dump_ext_part_header(struct mtk_part_header_extension *ext);

static u32 get_aligned_part_size(struct mtk_partition_header_data *hdr);
static u64 get_full_part_size(struct mtk_partition_header_data *hdr);
static u64 get_full_aligned_part_size(struct mtk_partition_header_data *hdr);
static void * get_full_memory_address(struct mtk_partition_header_data *hdr);
static const char *get_img_type_string(u32 img_type);

int main(int argc, char **argv)
{
    if (setup_log()) {
        fprintf(stderr, "Log setup failed. Stop.\n");
        return EXIT_FAILURE;
    }

    s_log_debug(MODULE_NAME);

    if (argc <= 1 || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
        s_log_error("Not enough arguments");
        s_log_info("Usage: " MODULE_NAME " <FILE1> [FILE2 FILE3 ...]");
        goto err;
    }

    for (i32 i = 1; i < argc; i++) {
        const char *path = argv[i];

        FILE *fp = fopen(path, "rb");
        if (fp == NULL) {
            s_log_error("Failed to open \"%s\": %s", path, strerror(errno));
            goto err;
        }
        s_log_verbose("Processing file \"%s\"...", path);

        union mtk_partition_header hdr = { 0 };
        i32 ret = fread(&hdr.buf_, MTK_PART_HEADER_SIZE, 1, fp);
        if (ret == -1 && ferror(fp)) {
            s_log_error("Failed to fread() the header intro from \"%s\": %s",
                path, strerror(errno));
            goto cleanup_file;
        } else if (ret == -1 && feof(fp)) {
            s_log_error("File \"%s\" too small (end of file reached)", path);
            goto cleanup_file;
        } else if (ret != 1) {
            s_log_error("Read an incorrect amount of bytes from \"%s\" "
                "(read: %#x, expected: %#x)",
                path,
                ret * MTK_PART_HEADER_SIZE, MTK_PART_HEADER_SIZE
            );
            goto cleanup_file;
        }

        if (hdr.data.magic != MTK_PART_MAGIC) {
            s_log_error("Invalid magic: 0x%.8x (expected: 0x%.8x)",
                hdr.data.magic, MTK_PART_MAGIC);
            goto cleanup_file;
        }

        dump_part_header(&hdr.data);

cleanup_file:
        s_log_verbose("Done processing \"%s\"", path);
        if (fclose(fp))
            s_log_error("Failed to close \"%s\": %s", path, strerror(errno));
    }

    s_log_verbose("Exiting with code EXIT_SUCCESS");
    s_log_cleanup_all();
    return EXIT_SUCCESS;

err:
    s_log_error("Exiting with code EXIT_FAILURE");
    s_log_cleanup_all();
    return EXIT_FAILURE;
}

static i32 setup_log(void) {
    struct s_log_output_cfg cfg = {
        .type = S_LOG_OUTPUT_FILE,
        .out.file = stdout,
        .flags = S_LOG_CONFIG_FLAG_COPY
    };

    if (s_configure_log_outputs(S_LOG_STDOUT_MASKS, &cfg)) {
        fprintf(stderr, "Failed to configure stdout log outputs\n");
        return 1;
    }

    cfg.out.file = stderr;
    if (s_configure_log_outputs(S_LOG_STDERR_MASKS, &cfg)) {
        fprintf(stderr, "Failed to configure stderr log outputs\n");
        return 1;
    }

    s_configure_log_level(S_LOG_INFO);

    return 0;
}

#define log_magic(prepend_str, magic) s_log_info(                   \
        "%s0x%.8x, // (BE: 0x%.2x%.2x%.2x%.2x)", prepend_str, magic,\
        (u8)((magic & 0x000000FF) >> 0), /* convert endianness */   \
        (u8)((magic & 0x0000FF00) >> 8),                            \
        (u8)((magic & 0x00FF0000) >> 16),                           \
        (u8)((magic & 0xFF000000) >> 24)                            \
    )

static void dump_part_header(struct mtk_partition_header_data *hdr)
{
    const char *old_line_verbose = NULL, *old_line_info = NULL;
    s_configure_log_line(S_LOG_VERBOSE, "%s\n", &old_line_verbose);
    s_configure_log_line(S_LOG_INFO, "%s\n", &old_line_info);

    s_log_verbose("===== Begin Mediatek partition header dump =====");
    s_log_info("union mtk_partition_header hdr = {");
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
    dump_ext_part_header(&hdr->ext);
    s_log_info("        }");
    s_log_info("    }");
    s_log_info("};");
    s_log_verbose("=====  End Mediatek partition header dump  =====");

    s_configure_log_line(S_LOG_VERBOSE, old_line_verbose, NULL);
    s_configure_log_line(S_LOG_INFO, old_line_info, NULL);
}

static void dump_ext_part_header(struct mtk_part_header_extension *ext)
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

static u32 get_aligned_part_size(struct mtk_partition_header_data *hdr)
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

static u64 get_full_part_size(struct mtk_partition_header_data *hdr)
{
    if (hdr->ext.magic == MTK_PART_EXT_MAGIC) {
        const u64 high = (u64)hdr->ext.part_size_hi << 32;
        return high | hdr->part_size;
    } else {
        return (u64)hdr->part_size;
    }
}

static u64 get_full_aligned_part_size(struct mtk_partition_header_data *hdr)
{
    const u32 low = get_aligned_part_size(hdr);
    const u64 high = get_full_part_size(hdr) & 0xFFFFFFFF00000000ULL;

    return high | low;
}

static void * get_full_memory_address(struct mtk_partition_header_data *hdr)
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
