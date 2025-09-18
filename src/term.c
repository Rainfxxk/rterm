#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "config.h"
#include "term.h"
#include "util.h"

// typedef struct term_t {
//     int r, c;
//     int cur_x, cur_y;
//     int just_wraped;
//     char **screen;
//     arg_t arg;
//     unsigned num_par;
//     unsigned par[PARAMETER_NUM];
// }term_t ;
void *_callback(int, void *) {
    return NULL;
}

void reset_paser(term_t *term) {
    term->paser.ps = 0;
    memset(term->paser.pm, 0, PM_NUM * sizeof(unsigned));
    memset(term->paser.pt, 0, PT_LEN * sizeof(char));
    term->paser.pt_len = 0;
    term->paser.num_par = 0;
    term->paser.state = NORMAL;
}

term_t *get_term(int r, int c) {
    term_t *term = (term_t *)CHECK_PTR(malloc(sizeof(term_t)), "no memory for malloc a term_t");

    term->r = r;
    term->c = c;

    term->cur_x = term->cur_y = 0;
    term->just_wraped = 0;
    term->screen = (tchar_t **)CHECK_PTR(malloc(r * sizeof(char *)), "no memory for malloc screen");
    term->arg.bg = TERM_BACKGROUND;
    term->arg.fg = TERM_FOREGROUND;
    for (int j = 0; j < r; j++) {
        *(term->screen + j) = (tchar_t *)CHECK_PTR(malloc(c * sizeof(tchar_t) + 1), "no memory for malloc screen");
        // memset(term->screen[i], '\0', (c + 1) * sizeof(tchar_t));
        for (int i = 0; i < c; i++) {
            term->screen[j][i].ch = '\0';
            term->screen[j][i].arg = term->arg;
        }
    }

    reset_paser(term);
    term->callback = _callback;

    return term;
}

void set_color(term_t *term) {
    if (term->paser.num_par == 0) {
        term->arg.bg = TERM_BACKGROUND; term->arg.bg = TERM_FOREGROUND;
    }
    else for(unsigned i = 0; i < term->paser.num_par; i++) {
        unsigned par = term->paser.pm[i];
        if (par == 0) {term->arg.bg = TERM_BACKGROUND; 
                       term->arg.fg = TERM_FOREGROUND;}
        if (par == 30) term->arg.fg = TERM_BLACK;
        if (par == 31) term->arg.fg = TERM_RED;
        if (par == 32) {term->arg.fg = TERM_GREEN;}
        if (par == 33) term->arg.fg = TERM_YELLOW;
        if (par == 34) term->arg.fg = TERM_BLUE;
        if (par == 35) term->arg.fg = TERM_MAGENTA;
        if (par == 36) term->arg.fg = TERM_CYAN;
        if (par == 37) term->arg.fg = TERM_WHITE;
        if (par == 39) term->arg.fg = TERM_FOREGROUND;
        if (par == 40) term->arg.bg = TERM_BLACK;
        if (par == 41) term->arg.bg = TERM_RED;
        if (par == 42) term->arg.bg = TERM_GREEN;
        if (par == 43) term->arg.bg = TERM_YELLOW;
        if (par == 44) term->arg.bg = TERM_BLUE;
        if (par == 45) term->arg.bg = TERM_MAGENTA;
        if (par == 46) term->arg.bg = TERM_CYAN;
        if (par == 47) term->arg.bg = TERM_WHITE;
        if (par == 49) term->arg.bg = TERM_FOREGROUND;
        term->paser.pm[i] = 0;
    }
    term->paser.num_par = 0;
}

void cursor_control(term_t *term, char func) {
    int first = term->paser.pm[0] == 0? 1 : term->paser.pm[0];
    int second = term->paser.pm[1] == 0? 1 : term->paser.pm[1];
    // printf("cursor %d %d\n", first, second);
    // printf("cursor position: %d %d\n", term->cur_y, term->cur_x);
    switch (func) {
        CASE('A', term->cur_y = MIN(0,           term->cur_y - first););
        CASE('B', term->cur_y = MAX(term->r - 1, term->cur_y + first););
        CASE('C', term->cur_x = MIN(term->c - 1, term->cur_x + first););
        CASE('D', term->cur_x = MAX(0,           term->cur_x - first););
        CASE('E', term->cur_y = MAX(0,           term->cur_y - first););
        CASE('F', term->cur_y = MIN(term->r - 1, term->cur_y + first););
        CASE('G', term->cur_y = MIN(term->r - 1,   MAX(0, first - 1)););
        CASE('H', term->cur_y = MIN(term->r - 1,   MAX(0, first - 1));
                  term->cur_x = MIN(term->c - 1,  MAX(0, second - 1)););
    }
    
}

void _erase_line(term_t *term, int r, int c, int func) {
    arg_t arg = {.bg = TERM_BACKGROUND, .fg = TERM_FOREGROUND};
    tchar_t erase = {.ch = ' ', arg = arg};

    switch (func) {
        CASE(0, for(int i = c; i < term->c; i++) { term->screen[r][i] = erase;});
        CASE(1, for(int i = 0; i < c;       i++) { term->screen[r][i] = erase;});
        CASE(2, for(int i = 0; i < term->c; i++) { term->screen[r][i] = erase;});
    }
}

void erase_line(term_t *term) {
    int func = term->paser.pm[0];
    _erase_line(term, term->cur_y, term->cur_x, func);
}

void erase_display(term_t *term) {

    // this is a very interesting wry to erase screen, at least I think so.
    // as the ansi standard describe:
    // 0(0b000) for erase after the cursor
    // 1(0b001) for erase before the cursor
    // 2(0b010) for erase the whole screen
    // 3(0b011) for erase the sceen and buffer
    // but when func plus 1, this ganna happen:
    // 1(0b001) for erase after the cursor
    // 2(0b010) for erase before the cursor
    // 3(0b011) for erase the whole screen
    // 4(0b100) for erase the sceen and buffer
    // then you will found:
    // 0th bit for erase after the cursor
    // 1th bit for erase after the cursor
    // 2th bit for erase every thing
    
    int func = term->paser.pm[0] + 1;
    if (func        & 0b1) for (int i = term->cur_y; i < term->r; i++) _erase_line(term, i, term->cur_x, i == term->cur_y? 0: 2);
    if ((func >> 1) & 0b1) for (int i = term->cur_y; i > -1;      i--) _erase_line(term, i, term->cur_x, i == term->cur_y? 1: 2);
    if ((func >> 2) & 0b1) for (int i = 0;           i < term->r; i++) _erase_line(term, i, 0, 2);

}

void osc(term_t *term) {
    if (term->paser.ps == 0) {
        char *title = malloc((term->paser.pt_len + 1) * sizeof(char));
        strcpy(title, term->paser.pt);
        term->callback(SET_TITLE, title);
    }
}

int handle_ansi(term_t *term, const char ch) {
    #define STATE(STATE, CODE)      if (term->paser.state == STATE) {CODE; reset_paser(term); return 0;}
    #define NORMAL_STATE(CODE)      STATE(NORMAL, CODE)
    #define ESCAPE_STATE(CODE)      STATE(ESCAPE, CODE)
    #define CSI_STATE(CODE)    STATE(CSI, CODE)
    #define OSC_STATE(CODE)    STATE(OSC, CODE)

    NORMAL_STATE(
        if (ch == ANSI_BELL       ) { return 1;                                                                 }
        if (ch == ANSI_BACKSPACE  ) { if (--term->cur_x < 0) { term->cur_y--; term->cur_x = term->c - 2;}
                                      term->screen[term->cur_y][term->cur_x].ch = '\0'; return 1;               }
        if (ch == ANSI_NEWLINE    ) { if(!term->just_wraped) { term->cur_y++; term->just_wraped = 0; } return 1;}
        if (ch == ANSI_RETURN     ) { term->cur_x = 0; return 1;                                                }
        if (ch == ANSI_ESCAPE     ) { term->paser.state = ESCAPE; return 1;                                     }
    );
    ESCAPE_STATE(
        if (ch == '['             ) { term->paser.state = CSI; return 1;                                        }
        if (ch == ']'             ) { term->paser.state = OSC; return 1;                                        }
    );
    CSI_STATE(
        if (ch == ';'             ) { term->paser.num_par++; return 1;                                          }
        if (ch >= '<' && ch <= '?') { term->paser.pm[term->paser.num_par++] = ch; return 1;                     }
        if (ch >= '0' && ch <= '9') { int n = term->paser.num_par;
                                      term->paser.pm[n] = term->paser.pm[n] * 10 + ch - '0'; 
                                      return 1;                                                                 }
        if (ch == 'm'             ) { term->paser.num_par++;
                                      set_color(term);          reset_paser(term); return 1;                    }
        if (ch >= 'A' && ch <= 'H') { term->paser.num_par++;
                                      cursor_control(term, ch); reset_paser(term); return 1;                    }
        if (ch == 'J'             ) { term->paser.num_par++;
                                      erase_display(term);      reset_paser(term); return 1;                    }
        if (ch == 'K'             ) { term->paser.num_par++;
                                      erase_line(term);         reset_paser(term); return 1;                    }
        if (ch == 'h'             ) { reset_paser(term); return 1;}
        if (ch == 'l'             ) { reset_paser(term); return 1;}
    );
    OSC_STATE(
        if (ch == ';'             ) { term->paser.num_par++; return 1;                                          }
        if (ch >= '0' && ch <= '9') { if (term->paser.num_par == 0) {
                                          int n = term->paser.num_par;
                                          term->paser.pm[n] = term->paser.pm[n] * 10 + ch - '0'; 
                                          return 1;}                                                            }
        if (ch == ANSI_BELL       ) { osc(term); reset_paser(term); return 1;                                   }
        if (ch == ANSI_ST         ) { osc(term); reset_paser(term); return 1;                                   }
        else if (term->paser.num_par == 1) { term->paser.pt[term->paser.pt_len++] = ch; return 1;               }
    );

    reset_paser(term);
    return 0;
}

void clearline(tchar_t *line, int n) {
    for (int i = 0; i < n; i++) {
        (line + i)->ch = '\0';
    }
}

void screen_up(term_t *term, int n) {
    tchar_t *screen_temp[term->r];
    for (int i = 0; i < term->r; i++) screen_temp[i] = term->screen[i];
    for (int i = 0; i < term->r - n; i++ ) term->screen[i] = screen_temp[i + n];
    for (int i = term->r - n; i < term->r; i++) {
        term->screen[i] = screen_temp[term->r - n - i];
        clearline(term->screen[i], term->c);
    }
}

int term_write(term_t *term, const char *str) {
    int len = strlen(str);
    int y = term->cur_y;

    for (int i = 0; i < len; i++) {
        // printf("(%d %d): ", term->cur_y, term->cur_x);
        // printf("%c", *(str + i));
        // if (*(str + i) == '\x1b') {
        //     printf("%\\x1b(%d)\n", *(str + i), *(str + i));
        // }
        // else {
        //     printf("%c(%d)\n", *(str + i), *(str + i));
        // }
        // fflush(stdout);
        int res = handle_ansi(term, *(str + i));
        if ( res == 0) {
            term->screen[term->cur_y][term->cur_x].ch = *(str + i);
            term->screen[term->cur_y][term->cur_x++].arg = term->arg;
        }

        if (term->cur_x >= term->c) {
            term->cur_x = 0;
            term->cur_y++;
            term->just_wraped = 1;
        }
        else {
            term->just_wraped = 0;
        }
        if (term->cur_x < 0) {
            term->cur_y--;
            term->cur_x = term->c - 2;
        }

        if (term->cur_y >= term->r) {
            screen_up(term, term->cur_y - term->r + 1);
            term->cur_y = term->r - 1;
        }
    }

    term->callback(REDRAW, NULL);

    return term->cur_y - y + 1;
}
