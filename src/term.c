#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

    term->paser.state = STATE_NORMAL;
    term->paser.num_par = 0;
    for (int i = 0; i < PARAMETER_NUM; i++) term->paser.par[i] = 0;

    return term;
}

void set_color(term_t *term) {
    if (term->paser.num_par == 0) {
        term->arg.bg = TERM_BACKGROUND; term->arg.bg = TERM_FOREGROUND;
    }
    else for(unsigned i = 0; i < term->paser.num_par; i++) {
        unsigned par = term->paser.par[i];
        if (par == 0) {term->arg.bg = TERM_BACKGROUND; term->arg.fg = TERM_FOREGROUND;}
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
        term->paser.par[i] = 0;
    }
    term->paser.num_par = 0;
}

void reset_paser(term_t *term) {
    memset(term->paser.par, 0, PARAMETER_NUM);
    term->paser.num_par = 0;
    term->paser.state = STATE_NORMAL;
}

int handle_ansi(term_t *term, const char ch) {
    #define STATE(STATE, CODE)      if (term->paser.state == STATE) {CODE; return 1;} break; 
    #define NORMAL_STATE(CODE)      STATE(STATE_NORMAL, CODE)
    #define ESCAPE_STATE(CODE)      STATE(STATE_ESCAPE, CODE)
    #define ARGUMENT_STATE(CODE)    STATE(STATE_ARGUMENT, CODE)

    // if (ch == ANSI_ESCAPE) printf("^[");
    // else printf("%c", ch); 
    switch(ch) {
        CASE(ANSI_BELL, return 1;);
        CASE(ANSI_BACKSPACE, term->screen[term->cur_y][--term->cur_x].ch = '\0'; return 1;);
        CASE(ANSI_NEWLINE, if(!term->just_wraped) { term->cur_y++; term->just_wraped = 0; } return 1;);
        CASE(ANSI_RETURN, term->cur_x = 0; return 1;);
        CASE(ANSI_ESCAPE, term->paser.state = STATE_ESCAPE; return 1;);
        case '[':  ESCAPE_STATE(term->paser.state = STATE_ARGUMENT; return 1;);
        case ';':  ARGUMENT_STATE(term->paser.num_par++; return 1;)
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '0': ARGUMENT_STATE(term->paser.state = STATE_ARGUMENT; term->paser.par[term->paser.num_par] = term->paser.par[term->paser.num_par] * 10 + ch - '0'; return 1; )
        case 'm': ARGUMENT_STATE(term->paser.num_par++; set_color(term); reset_paser(term); return 1;)
        // case 'A': ARGUMENT_STATE(; return 1;);
        // case 'B': ARGUMENT_STATE(; return 1;);
        // case 'C': ARGUMENT_STATE(; return 1;);
        // case 'D': ARGUMENT_STATE(; return 1;);
        // case 'E': ARGUMENT_STATE(; return 1;);
        // case 'F': ARGUMENT_STATE(; return 1;);
        // case 'G': ARGUMENT_STATE(; return 1;);
        // case 'H': ARGUMENT_STATE(; return 1;);
    }
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
        int res = handle_ansi(term, *(str + i));
        if ( res == 0) {
            term->screen[term->cur_y][term->cur_x].ch = *(str + i);
            term->screen[term->cur_y][term->cur_x++].arg = term->arg;
            if (term->cur_x >= term->c) {
                term->cur_x = 0;
                term->cur_y++;
                term->just_wraped = 1;
            }
            else {
                term->just_wraped = 0;
            }
        }

        if (term->cur_y >= term->r) {
            screen_up(term, term->cur_y - term->r + 1);
            term->cur_y = term->r - 1;
        }
    }

    return term->cur_y - y + 1;
}
