#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "config.h"
#include "term.h"
#include "util.h"

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

    for (int j = 0; j < r; j++) {
        *(term->screen + j) = (tchar_t *)CHECK_PTR(malloc((c + 1) * sizeof(tchar_t)), "no memory for malloc screen");
        // memset(term->screen[i], '\0', (c + 1) * sizeof(tchar_t));
        for (int i = 0; i < c; i++) {
            term->screen[j][i].ch = ' ';
            term->screen[j][i].arg = term->arg;
        }
        term->screen[j][term->c].ch = '\0';
    }

    printf("term size: %ld", sizeof(term_t) + r * (c + 1) * sizeof(tchar_t));

    term->default_arg.bg = TERM_BACKGROUND;
    term->default_arg.fg = TERM_FOREGROUND;
    term->arg = term->default_arg;
    reset_paser(term);

    term->scroll_top = 0;
    term->scroll_bottom = term->r - 1;

    term->callback = _callback;

    return term;
}

void clearline(term_t *term, int r) {
    printf("clearline %d\n", r);
    for (int i = 0; i < term->c; i++) {
        term->screen[r][i].ch = ' ';
        term->screen[r][i].arg = term->default_arg;
    }
}

void copy_lines(term_t *term, unsigned int dst, unsigned int src, unsigned int n) {
    if (src > term->scroll_bottom ||
        dst > term->scroll_bottom ||
        src == dst    ||    n  <= 0) return;

    n = MIN(n, term->scroll_bottom - src + 1);
    tchar_t screen_temp[n][term->c + 1];

    for (unsigned int i = 0; i < n; i++) {
        memcpy(screen_temp[i], term->screen[src + i], sizeof(tchar_t) * (term->c + 1));
    }

    n = MIN(n, term->scroll_bottom - dst + 1);
    for (unsigned int i = 0; i < n; i++) {
        memcpy(term->screen[dst + i], screen_temp[i], sizeof(tchar_t) * (term->c + 1));
    }
}

void _scroll_up(term_t *term, unsigned int n) {
    if (n == 0 || n > term->scroll_bottom - term->scroll_top) return;

    unsigned int src = term->scroll_top + n;
    unsigned int dst = term->scroll_top;
    unsigned copy_n = term->scroll_bottom - src + 1;
    copy_lines(term, dst, src, copy_n);

    for (unsigned int i = term->scroll_bottom - n + 1; i <= term->scroll_bottom; i++) {
        clearline(term, i);
    }

    return;
}

void _scroll_down(term_t *term, unsigned int n) {
    if (n == 0 || n > term->scroll_bottom - term->scroll_top) return;

    unsigned int src = term->scroll_top;
    unsigned int dst = n;
    unsigned copy_n = term->scroll_bottom - src + 1;
    copy_lines(term, dst, src, copy_n);

    for (unsigned int i = 0; i < n; i++) {
        clearline(term, i);
    }

    return;
}

void set_color(term_t *term) {
    term->paser.num_par++;
    if (term->paser.num_par == 0) {
        term->arg = term->default_arg;
    }
    else for(unsigned i = 0; i < term->paser.num_par; i++) {
        unsigned par = term->paser.pm[i];
        if (par == 00) term->arg = term->default_arg; 
        if (par == 30) term->arg.fg = TERM_BLACK;
        if (par == 31) term->arg.fg = TERM_RED;
        if (par == 32) term->arg.fg = TERM_GREEN;
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
    }
}

void cursor_control(term_t *term, char func) {
    int first = term->paser.pm[0] == 0? 1 : term->paser.pm[0];
    int second = term->paser.pm[1] == 0? 1 : term->paser.pm[1];

    switch (func) {
        CASE('A', term->cur_y = MIN(0,           term->cur_y - first););
        CASE('B', term->cur_y = MAX(term->r - 1, term->cur_y + first););
        CASE('C', term->cur_x = MIN(term->c - 1, term->cur_x + first););
        CASE('D', term->cur_x = MAX(0,           term->cur_x - first););
        CASE('E', term->cur_y = MAX(0,           term->cur_y - first););
        CASE('F', term->cur_y = MIN(term->r - 1, term->cur_y + first););
        CASE('G', term->cur_x = MIN(term->c - 1,   MAX(0, first - 1)););
        CASE('H', term->cur_y = MIN(term->r - 1,   MAX(0, first - 1));
                  term->cur_x = MIN(term->c - 1,  MAX(0, second - 1)););
    }
    
}

void erase_char(term_t *term) {
    int n = MIN((unsigned int)(term->c - term->cur_x), MAX(1u, term->paser.pm[0]));
    tchar_t erase = {.ch = ' ', .arg = term->default_arg};

    for (int i = 0; i < n; i++) {
        term->screen[term->cur_y][term->cur_x + i] = erase;
    }
}

void _erase_line(term_t *term, int r, int c, int func) {
    tchar_t erase = {.ch = ' ', .arg = term->default_arg};

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

void insert_blank(term_t *term) {
    int len = term->paser.pm[0] <= 0? 1 : term->paser.pm[0];
    char *blank = (char *)CHECK_PTR(malloc((len + 1) * sizeof(char)), "no memory for blank");
    blank[len] = '\0';
    memset(blank, ' ', len);
    term_write(term, blank);
    free(blank);
}

void insert_line(term_t *term) {
    unsigned n = term->paser.pm[0] == 0? 1 : term->paser.pm[0];

    unsigned src = term->cur_y;
    unsigned dst = MIN(term->scroll_bottom, term->cur_y + n);
    unsigned copy_n = term->scroll_bottom - term->cur_y + 1;

    copy_lines(term, dst, src, copy_n);
    for (unsigned i = 0; i < n; i++) {
        clearline(term, term->cur_y + i);
    }
    term->cur_x = 0;
}

void delete_line(term_t *term) {
    int n = term->paser.pm[0] <= 0? 1 : term->paser.pm[0];
    n = MIN(n, term->r - term->cur_y);
    for (int i = 0; i < n; i++) {
        _erase_line(term, term->cur_y + i, term->cur_x, 2);
    }
    for (int i = term->cur_y + n; i < term->r; i++) {
        memcpy(term->screen[i - 1], term->screen[i], term->c * sizeof(tchar_t));
    }
}

void delete_char(term_t *term) {
    int n = term->paser.pm[0] <= 0? 1 : term->paser.pm[0];
    n = MIN(n, term->c - term->cur_x);
    memmove(term->screen[term->cur_y] + term->cur_x,
            term->screen[term->cur_y] + term->cur_x + n,
            term->c - term->cur_x - n);
    for (int i = 0; i < n; i++) {
        term->screen[term->cur_y][term->c - i - 1].ch = '\0';
        term->screen[term->cur_y][term->c - i - 1].arg = term->default_arg;
    }
}

void scroll_up(term_t *term) {
    int n = term->paser.pm[0] <= 0? 1 : term->paser.pm[0];
    tchar_t *screen_temp[term->r];

    for (int i = 0; i < term->r; i++) screen_temp[i] = term->screen[i];
    for (int i = 0; i < term->r - n; i++ ) term->screen[i] = screen_temp[i + n];
    for (int i = term->r - n; i < term->r; i++) {
        term->screen[i] = screen_temp[i - term->r + n];
        clearline(term, i);
    }
}

void scroll_down(term_t *term) {
    int n = term->paser.pm[0] <= 0? 1 : term->paser.pm[0];
    tchar_t *screen_temp[term->r];

    for (int i = 0; i < term->r; i++) screen_temp[i] = term->screen[i];
    for (int i = 0; i < term->r - n; i++ ) term->screen[i + n] = screen_temp[i];
    for (int i = term->r - n; i < term->r; i++) {
        term->screen[i - term->r + n] = screen_temp[i];
        clearline(term, i);
    }
}

void backword_tabulation(term_t *term) {
    int n = term->paser.pm[0] <= 0? 1 : term->paser.pm[0];
    n = MIN(term->cur_x / 8, n);
    int tab_num = ( term->cur_x / 8 ) - n;
    tab_num = term->cur_x % 8 == 0 ? tab_num : tab_num + 1;
    term->cur_x = tab_num * 8;
}

void set_cur_x(term_t *term) {
    term->cur_x = MIN((unsigned int)(term->c - 1), term->paser.pm[0] - 1);
}

void scroll_region(term_t *term) {
    unsigned int top = MIN((unsigned)term->r - 1, term->paser.pm[0] - 1);
    unsigned int bottom = term->r - 1;
    if (term->paser.num_par > 0) {
        bottom = MIN((unsigned)term->r - 1, MAX(top, term->paser.pm[1] - 1));
    }
    term->scroll_top = top;
    term->scroll_bottom = bottom;

    term->cur_y = top;
    term->cur_x = 0;
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
        if (ch == ANSI_BELL       ) { return 1;                                                         }
        if (ch == ANSI_BACKSPACE  ) { if (--term->cur_x < 0) { term->cur_y--; term->cur_x = term->c - 2;}
                                      term->screen[term->cur_y][term->cur_x].ch = ' '; return 1;        }
        if (ch == ANSI_NEWLINE    ) { if(!term->just_wraped) { term->cur_y++; term->just_wraped = 0;    } 
                                      return 1;                                                         }
        if (ch == ANSI_RETURN     ) { term->cur_x = 0; return 1;                                        }
        if (ch == ANSI_ESCAPE     ) { term->paser.state = ESCAPE; return 1;                             }
    );
    ESCAPE_STATE(
        if (ch == '['             ) { term->paser.state = CSI; return 1;                                }
        if (ch == ']'             ) { term->paser.state = OSC; return 1;                                }
    );
    CSI_STATE(
        if (ch == ';'             ) { term->paser.num_par++; return 1;                                  }
        if (ch >= '<' && ch <= '?') { term->paser.pm[term->paser.num_par++] = ch; return 1;             }
        if (ch >= '0' && ch <= '9') { int n = term->paser.num_par;
                                      term->paser.pm[n] = term->paser.pm[n] * 10 + ch - '0'; 
                                      return 1;                                                         }
        if (ch == '@'             ) { insert_blank(term);       reset_paser(term); return 1;            }
        if (ch >= 'A' && ch <= 'H') { cursor_control(term, ch); reset_paser(term); return 1;            }
        if (ch == 'J'             ) { erase_display(term);      reset_paser(term); return 1;            }
        if (ch == 'K'             ) { erase_line(term);         reset_paser(term); return 1;            }
        if (ch == 'L'             ) { insert_line(term);        reset_paser(term); return 1;            }
        if (ch == 'M'             ) { delete_line(term);        reset_paser(term); return 1;            }
        if (ch == 'P'             ) { delete_char(term);        reset_paser(term); return 1;            }
        if (ch == 'S'             ) { scroll_up(term);          reset_paser(term); return 1;            }
        if (ch == 'T'             ) { scroll_down(term);        reset_paser(term); return 1;            }
        if (ch == 'X'             ) { erase_char(term);         reset_paser(term); return 1;            } 
        if (ch == 'Z'             ) { backword_tabulation(term);reset_paser(term); return 1;            } 
        if (ch == '`'             ) { set_cur_x(term);          reset_paser(term); return 1;            } 
        if (ch == 'h'             ) { reset_paser(term); return 1;}
        if (ch == 'l'             ) { reset_paser(term); return 1;}
        if (ch == 'm'             ) { set_color(term); reset_paser(term); return 1;                     }
        if (ch == 'r'             ) { scroll_region(term); reset_paser(term); return 1;                 }
        if (ch >= 'a' && ch <= 'z') {reset_paser(term); return 1;}
    );
    OSC_STATE(
        if (ch == ';'             ) { term->paser.num_par++; return 1;                                  }
        if (ch >= '0' && ch <= '9') { if (term->paser.num_par == 0) {
                                          int n = term->paser.num_par;
                                          term->paser.pm[n] = term->paser.pm[n] * 10 + ch - '0'; 
                                          return 1;}                                                    }
        if (ch == ANSI_BELL       ) { osc(term); reset_paser(term); return 1;                           }
        if (ch == ANSI_ST         ) { osc(term); reset_paser(term); return 1;                           }
        else if (term->paser.num_par == 1) { term->paser.pt[term->paser.pt_len++] = ch; return 1;       }
    );

    reset_paser(term);
    return 0;
}

int term_write(term_t *term, const char *str) {
    int len = strlen(str);
    int y = term->cur_y;

    char control_seq[100];
    char index = 0;
    int flag = 0;

    for (int i = 0; i < len; i++) {
        int res = handle_ansi(term, *(str + i));
        if ( res == 0) {
            if (flag == 1) {
                control_seq[index] = '\0';
                for (int i = 0; i < index; i++) {
                    if (*(str + i) == '\x1b') {
                        printf("\\x1b", control_seq[i]);
                    }
                    else {
                        printf("%c", control_seq[i]);
                    }
                }
                printf("\n");
                fflush(stdout);
                index = 0;
                flag = 0;
            }
            printf("%c", *(str + i));
            term->screen[term->cur_y][term->cur_x].ch = *(str + i);
            term->screen[term->cur_y][term->cur_x++].arg = term->arg;
        }
        else {
            if (flag == 0) {
                printf("\n");
                flag = 1;
            }
            else if (flag == 1 && *(str + i) == '\x1b'){
                control_seq[99] = '\0';
                for (int i = 0; i < index; i++) {
                    if (*(str + i) == '\x1b') {
                        printf("\\x1b", control_seq[i]);
                    }
                    else {
                        printf("%c", control_seq[i]);
                    }
                }
                printf("\n");
                fflush(stdout);
                index = 0;
                flag = 0;
            }
            control_seq[index++] = *(str + i);
            if (index >= 100) {
                control_seq[99] = '\0';
                for (int i = 0; i < index; i++) {
                    if (*(str + i) == '\x1b') {
                        printf("\\x1b", control_seq[i]);
                    }
                    else {
                        printf("%c", control_seq[i]);
                    }
                }
                printf("\n");
                fflush(stdout);
                index = 0;
                flag = 0;
            }
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
            term->cur_x = term->c - 1;
        }

        if ((unsigned)term->cur_y > term->scroll_bottom) {
            printf("scroll_up: %d\n", term->cur_y - term->scroll_bottom);
            _scroll_up(term, term->cur_y - term->scroll_bottom);
            term->cur_y = term->scroll_bottom;
        }
    }

    term->callback(REDRAW, NULL);

    return term->cur_y - y + 1;
}
