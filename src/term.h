#ifndef __TERM_H__
#define __TERM_H__

#include <pthread.h>
#include "config.h"
#include "pty.h"

typedef struct arg_t {
    unsigned fg, bg;
    unsigned char reverse;
} arg_t;

typedef struct tchar_t {
    char ch;
    arg_t arg;
} tchar_t;

typedef struct tline_t {
    tchar_t *str;
    int is_wraped;
} tline_t;

typedef struct ansi_paser_t {
    unsigned num_par;
    unsigned ps;
    unsigned pm[PM_NUM];
    char pt[PT_LEN];
    unsigned pt_len;
    enum paser_state {
        NORMAL,
        ESCAPE,
        CSI,
        OSC,
    } state;
} ansi_paser_t;

enum callback_event {
    NONE = 0,
    REDRAW = 1,
    SET_TITLE = 2,
};

typedef struct term_t {
    pthread_mutex_t mutex;
    int r, c;
    pty_t pty;
    int cur_x, cur_y;
    int just_wraped;
    tline_t *screen;
    arg_t default_arg;
    arg_t arg;
    ansi_paser_t paser;
    unsigned scroll_top, scroll_bottom;
    void *(*callback)(int, void *);
}term_t ;

void _scroll_up(term_t *term, unsigned int n);

term_t *get_term(int r, int c);
void term_resize(term_t *term, int r, int c);
void term_write_ch(term_t *term, const char ch);
int term_write(term_t *term, const char *str);


#endif // !__TERM_H__
