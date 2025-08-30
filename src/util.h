#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdio.h>

#define RGBA_A(rgba) rgba & 0x000ff
#define RGBA_B(rgba) (rgba >> 8) & 0x000ff
#define RGBA_G(rgba) (rgba >> 16) & 0x000ff
#define RGBA_R(rgba) (rgba >> 24) & 0x000ff
#define RGBA(rgba) RGBA_R(rgba), RGBA_G(rgba), RGBA_B(rgba), RGBA_A(rgba)

#define CHECK(result, ...) \
    if (result < 0) { \
        fprintf(stderr, __VA_ARGS__); \
        exit(-1); \
    }

#define CHECK_PTR(create_ptr, ...) ({ \
    void *ptr = create_ptr; \
    if ((ptr) == NULL) { \
        fprintf(stderr, __VA_ARGS__); \
        exit(-1);\
    } \
    ptr; \
})

#endif // !__UTIL_H__
