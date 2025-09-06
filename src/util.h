#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdio.h>


#define MIN(a, b) (a > b? b : a)
#define MAX(a, b) (a > b? a : b)

#define SDL_REDRAW (SDL_USEREVENT + 1)

#define RGBA_A(rgba) rgba & 0x000ff
#define RGBA_B(rgba) (rgba >> 8) & 0x000ff
#define RGBA_G(rgba) (rgba >> 16) & 0x000ff
#define RGBA_R(rgba) (rgba >> 24) & 0x000ff
#define RGBA(rgba) RGBA_R(rgba), RGBA_G(rgba), RGBA_B(rgba), RGBA_A(rgba)

#define ANSI_BELL       '\x07'
#define ANSI_BACKSPACE  '\x08'
#define ANSI_TAB        '\x09'
#define ANSI_NEWLINE    '\x0a'
#define ANSI_RETURN     '\x0d'
#define ANSI_ESCAPE     '\x1b'

// cursor control
#define ANSI_UP         "\x1b[A"
#define ANSI_DOWN       "\x1b[B"
#define ANSI_RIGHT      "\x1b[C"
#define ANSI_LEFT       "\x1b[D"

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

#define CASE(value, code) case value: {{code;} break;}
#define CASE_NOBREAK(value, ...) \
    case value: \
    CASE_NOBREAK(##__VA_ARGS__)
    
#define CASE_SEQ(value, ...) CASE_NOBREAK()

#endif // !__UTIL_H__
