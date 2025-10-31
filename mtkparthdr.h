#ifndef MTK_PART_HEADER_H_
#define MTK_PART_HEADER_H_

/* Mediatek "partition header" */

#include <core/int.h>

#define MTK_PART_HEADER_SIZE 512

#define MTK_PART_MAGIC 0x58881688
#define MTK_PART_MAGIC_BE 0x88168858

#define MTK_PART_NAME_LEN 32

#define MTK_PART_EXT_MAGIC 0x58891689
#define MTK_PART_EXT_MAGIC_BE 0x89168958

#define MTK_PART_EXT_IMG_TYPE_LIST  \
    X_(AP_BIN,      0x00000000)     \
    X_(AP_BOOT_SIG, 0x00000001)     \
    X_(MODEM_LTE,   0x01000000)     \
    X_(MODEM_C2K,   0x01000001)     \
    X_(CERT1,       0x02000000)     \
    X_(CERT1_MODEM, 0x02000001)     \
    X_(CERT2,       0x02000002)     \


union mtk_partition_header {
	struct mtk_partition_header_data {
		u32 magic; /* Partition magic; always `MTK_PART_MAGIC` */
		u32 part_size; /* Size of partition */
		char part_name[32]; /* Partition name */
		u32 memory_address; /* Memory load address */
        u32 memory_address_mode; /* Whether `memory_address` is an offset
                                    from the start or the end */

        struct mtk_part_header_extension {
            u32 magic; /* Magic; always `MTK_PART_EXT_MAGIC` */
            u32 hdr_size; /* Always `MTK_PART_HEADER_SIZE` */
            u32 hdr_version; /* Header version */
            u32 img_type; /* Image type; see `MTK_PART_EXT_IMG_TYPE_LIST` */
            u32 is_image_list_end; /* Whether this image is the end of a list */
            u32 size_alignment_bytes; /* Image size alignment */
            u32 part_size_hi; /* High word of `data_size` on 64-bit */
            u32 memory_address_hi; /* high word of `memory_address` on 64-bit */
        } ext;
	} data;
	u8 buf_[MTK_PART_HEADER_SIZE];
};


#endif /* MTK_PART_HEADER_H_ */
