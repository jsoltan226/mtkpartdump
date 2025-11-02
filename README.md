# mtkpartdump - MediaTek partition dump tool

A small utility for inspecting and extracting the sub-partitions contained inside some MediaTek firmware binary blobs (e.g. `lk`, `md1img`, `tee1`)

## Background

MediaTek firmware images sometimes pack multiple logical images inside a single GPT partition (for example `lk`).
Each embedded image starts with a small MediaTek partition header, which describes its name, size, alignment, and other info.
These headers form a simple linked list of image entries stored sequentially within the same GPT partition.

Here's an example, simplified layout (from a real device):

```
|==================== GPT partition header ====================|
|                                                              |
|  |========================================================|  |
|  |                                                        |  |
|  |  other partitions, e.g. system, userdata, nvram, etc.  |  |
|  |                                                        |  |
|  |========================================================|  |
|                                                              |
|                                                              |
|  |=====================partition `lk`=====================|  |
|  |                                                        |  |
|  |  |--------------------------------------------------|  |  |
|  |  | MTK part header - name: "lk", size: ..., ...     |  |  |
|  |  |--------------------------------------------------|  |  |
|  |  | "lk" contents (2nd stage bootloader code)        |  |  |
|  |  | ...............                                  |  |  |
|  |  |--------------------------------------------------|  |  |
|  |  |--------------------------------------------------|  |  |
|  |  | MTK part header - name: "cert1", ...             |  |  |
|  |  |--------------------------------------------------|  |  |
|  |  | "cert1" contents (certificate 1 for lk) ...      |  |  |
|  |  |--------------------------------------------------|  |  |
|  |  |--------------------------------------------------|  |  |
|  |  | MTK part header - name: "cert2", list_end: 1, ...|  |  |
|  |  |--------------------------------------------------|  |  |
|  |  |--------------------------------------------------|  |  |
|  |  | "cert2" contents (certificate 2 for lk) .....    |  |  |
|  |  |--------------------------------------------------|  |  |
|  |                                                        |  |
|  |========================================================|  |
|                                                              |
|==============================================================|
```

### Header structure

The size of the header withing the binary is `0x200` (512 bytes),
but the actual struct spans only 80 bytes, and so the unused rest is just filled with `0xff` bytes.

The struct itself contains the following fields:
- Magic - `0x58881688`
- Partition size (32-bit)
- Partition name (32 byte-string)
- Partition load address (32-bit, often unused and set to a dummy value)
- `memory_address_mode` - offset mode flag (rarely used)
* An "extension" struct, containing even more useful info:
    - Another magic (`0x58891689`)
    - Header size (32-bit). Always set to `0x200`
    - Header version (32-bit). On my devices it's always `0x1`
    - A 32-bit value specifying the image's purpose. It can be one of the following:
        * `IMG_TYPE_AP_BIN` - Application processor (main CPU) binary executable
        * `IMG_TYPE_AP_BOOT_SIG` - Boot signature? (I'm not certain, haven't found it used anywhere)
        * `IMG_TYPE_MODEM_LTE` - LTE modem binary
        * `IMG_TYPE_MODEM_C2K` - CDMA2000 (3G) modem binary
        * `IMG_TYPE_CERT1` / `IMG_TYPE_CERT2` - Certificate images
        * `IMG_TYPE_CERT1_MODEM` - Certificate image for modem binaries (rarely used)
        Note that there might be more image types in newer devices.
    - `is_image_list_end` - the aforementioned `list_end` - if set to 1, signifies the end of the partition chain
    - Alignment (32-bit)
    - High part of partition size (32-bit) - Used when the partition size needs to be 64-bit
    - High part of the memory load address (32-bit) - Used when the memory load address needs to be 64-bit

### Example header chain
| offset     | type | content                                                  |
|------------|------|----------------------------------------------------------|
| [0x0]      | hdr  | (name: "lk", size: 0x204008, alignment: 0x10, is_end: 0) |
| [0x200]    | part | <LK code and data + 0x8 padding bytes>                   |
| [0x204210] | hdr  | (name: "cert1", size: 0x6ad, alignment: 0x10, is_end: 0) |
| [0x204410] | part | <cert1 contents + 0x3 padding bytes>                     |
| ...        |      |                                                          |

The list continues until a header with `is_image_list_end = 1` is found.

## Building/installation

Only a C11 compiler and `make` are required to build the project.
GCC and Clang are tested, but other compilers should work after at most minor tweaks in the `Makefile`.

Linux/unix:
```
git clone https://github.com/jsoltan226/mtkpartdump
cd mtkpartdump
PLATFORM=linux make release
```
The output executable can then be found at `bin/mtkpartdump`.

Windows (MSYS2/MinGW):
```
git clone https://github.com/jsoltan226/mtkpartdump
cd mtkpartdump
PLATFORM=windows make release
```
And the output executable will be in `bin\mtkpartdump.exe`.

You can configure the build by setting environment variables:
- `PLATFORM`: `linux` for linux & unix and `windows` for windows; default: `linux`
- `CC`: Path to custom C compiler; default: `cc`
- `CFLAGS`: Custom compiler flags
- `LDFLAGS`: Custom linker flags

For other build-time configuration options, see the `Makefile`.

## Usage

`mtkpartdump [OPTIONS...] <FILE1> [FILE2 FILE3 ...]`
Inspect and extract MediaTek logical partitions from firmware blobs.

Available options:
| Option                  | Description                                          |
| ----------------------- | ---------------------------------------------------- |
| `-h`, `--help`          | Show the help message and exit                       |
| `-V`, `--version`       | Print the program version and exit                   |
| `-v`, `--verbose`       | Enable verbose logging                               |
| `-c`, `--chain`         | Process all headers found in a partition chain       |
| `-s`, `--save-headers`  | Save raw binary partition headers to disk            |
| `-e`, `--extract-parts` | Extract binary partition contents                    |

Examples:
```
# List partitions in a single firmware blob
mtkpartdump lk.bin

# Process and extract all sub-partitions from one or more images
mtkpartdump -c -e lk.bin md1img.bin

# Save the header and content with verbose output
mtkpartdump -v -s -e md1img.bin
```

## Output
`mtkpartdump` parses and prints the contents of each found header, and, if requested,
extracts each sub-partition and/or its raw header into files named after the partition, in the current working directory.


## License
The project is licensed under [GPLv3+](./LICENSE).
Any contributions are welcome.
