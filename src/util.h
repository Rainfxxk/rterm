#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdio.h>
#include <assert.h>

#define SDL_REDRAW (SDL_USEREVENT + REDRAW)
#define SDL_SETTITLE (SDL_USEREVENT + SET_TITLE)

#define MIN(a, b) (a > b? b : a)
#define MAX(a, b) (a > b? a : b)

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
#define ANSI_ST         '\x9c'

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

#define ANSI_FG_BLACK   "\33[1;30m"
#define ANSI_FG_RED     "\33[1;31m"
#define ANSI_FG_GREEN   "\33[1;32m"
#define ANSI_FG_YELLOW  "\33[1;33m"
#define ANSI_FG_BLUE    "\33[1;34m"
#define ANSI_FG_MAGENTA "\33[1;35m"
#define ANSI_FG_CYAN    "\33[1;36m"
#define ANSI_FG_WHITE   "\33[1;37m"
#define ANSI_BG_BLACK   "\33[1;40m"
#define ANSI_BG_RED     "\33[1;41m"
#define ANSI_BG_GREEN   "\33[1;42m"
#define ANSI_BG_YELLOW  "\33[1;43m"
#define ANSI_BG_BLUE    "\33[1;44m"
#define ANSI_BG_MAGENTA "\33[1;45m"
#define ANSI_BG_CYAN    "\33[1;46m"
#define ANSI_BG_WHITE   "\33[1;47m"
#define ANSI_NONE       "\33[0m"

#define ANSI_FMT(str, fmt) fmt str ANSI_NONE


#define log(format, ...) \
    // printf(ANSI_FMT("[%s:%d %s] " format "\n", ANSI_FG_BLUE), __FILE__, __LINE__, __func__, ## __VA_ARGS__);

#define Assert(cond, format, ...) \
  do { \
    if (!(cond)) { \
        printf(ANSI_FMT("[%s:%d %s] " format "\n", ANSI_FG_RED), __FILE__, __LINE__, __func__,  ## __VA_ARGS__);\
      assert(cond); \
    } \
  } while (0)

#define panic(format, ...) Assert(0, format, ## __VA_ARGS__)

#endif // !__UTIL_H__
