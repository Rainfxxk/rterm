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
    term->screen = (char **)CHECK_PTR(malloc(r * sizeof(char *)), "no memory for malloc screen");
    for (int i = 0; i < r; i++) {
        *(term->screen + i) = (char *)CHECK_PTR(malloc(c * sizeof(char) + 1), "no memory for malloc screen");
        memset(term->screen[i], '\0', c + 1);
    }

    return term;
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

    int just_wraped = 0;
    for (int i = 0; i < len; i++) {
        if (*(str + i) == '\r') {
            term->cur_x = 0;
        }
        else if (*(str + i) != '\n') {
            term->screen[term->cur_y][term->cur_x++] = *(str + i);
            if (term->cur_x >= term->c) {
                term->cur_y++;
                just_wraped = 1;
            }
            else {
                just_wraped = 0;
            }
        }
        else if (!just_wraped) {
            term->cur_y++;
            just_wraped = 0;
        }

        if (term->cur_y >= term->r) {
            screen_up(term, term->cur_y - term->r + 1);
            term->cur_y = term->r - 1;
        }
    }

    return term->cur_y - y + 1;
}
