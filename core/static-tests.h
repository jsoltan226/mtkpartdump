#include <assert.h>

#ifndef __STDC__
#error Please use a standard C compiler!
#endif /* __STDC__ */

#if (__STDC_VERSION__ != 201112L)
#error Please use a C11 compiler (-std=c11)
#endif /* __STDC_VERSION__ */

#ifndef __STDC_HOSTED__
#error The C standard library implementation may be incomplete. \
    If you are sure that this is not the case, define `__STDC_HOSTED__` in your CFLAGS
#endif /* __STDC_HOSTED__ */

static_assert(sizeof(float) == 4, "Sizeof float32 must be 4 bytes (32 bits)");
static_assert(sizeof(double) == 8, "Sizeof float64 must be 8 bytes (64 bits)");

static_assert(sizeof(char) == 1, "Sizeof char must be 1 byte (8 bits)");
static_assert(sizeof(void *) == 8, "Sizeof void * must be 8 bytes (64 bits)");

static_assert(sizeof(void (*)(void)) == sizeof(void *),
    "The size of a function pointer must be the same "
    "as that of a normal (data) pointer");

#ifdef __STDC_NO_ATOMICS__
#error stdatomic.h support is required. Make sure you are compiling with
    a fully C11-compatible toolchain!
#endif /* STDC_NO_ATOMICS__ */
