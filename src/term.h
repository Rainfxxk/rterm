#ifndef __TERM_H__
#define __TERM_H__

#include "config.h"

typedef struct arg_t {
    unsigned fg, bg;
} arg_t;

typedef struct tchar_t {
    char ch;
    arg_t arg;
} tchar_t;

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
    int r, c;
    int cur_x, cur_y;
    int just_wraped;
    tchar_t **screen;
    arg_t arg;
    ansi_paser_t paser;
    void *(*callback)(int, void *);
}term_t ;

term_t *get_term(int r, int c);
int term_write(term_t *term, const char *str);


#endif // !__TERM_H__
