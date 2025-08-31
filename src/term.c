#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "term.h"
#include "util.h"

term_t *get_term(int r, int c) {
    term_t *term = (term_t *)CHECK_PTR(malloc(sizeof(term_t)), "no memory for malloc a term_t");

    term->r = r;
    term->c = c;

    term->cur_x = term->cur_y = 0;
    term->just_wraped = 0;
    term->screen = (char **)CHECK_PTR(malloc(r * sizeof(char *)), "no memory for malloc screen");
    for (int i = 0; i < r; i++) {
        *(term->screen + i) = (char *)CHECK_PTR(malloc(c * sizeof(char) + 1), "no memory for malloc screen");
        memset(term->screen[i], '\0', c + 1);
    }

    return term;
}

int handle_ansi(term_t *term, const char ch) {
    switch(ch) {
        CASE(ANSI_BELL, return 1;);
        CASE(ANSI_BACKSPACE, term->screen[term->cur_y][--term->cur_x] = '\0'; return 1;);
        CASE(ANSI_NEWLINE, if(!term->just_wraped) { term->cur_y++; term->just_wraped = 0; } return 1;);
        CASE(ANSI_RETURN, term->cur_x = 0; return 1;);
    }
    return 0;
}

void screen_up(term_t *term, int n) {
    char *screen_temp[term->r];
    for (int i = 0; i < term->r; i++) screen_temp[i] = term->screen[i];
    for (int i = 0; i < term->r - n; i++ ) term->screen[i] = screen_temp[i + n];
    for (int i = term->r - n; i < term->r; i++) {
        term->screen[i] = screen_temp[term->r - n - i];
        memset(term->screen[i], '\0', term->c);
    }
}

int term_write(term_t *term, const char *str) {
    int len = strlen(str);
    int y = term->cur_y;

    for (int i = 0; i < len; i++) {
        if (!handle_ansi(term, *(str + i))) {
            term->screen[term->cur_y][term->cur_x++] = *(str + i);
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
