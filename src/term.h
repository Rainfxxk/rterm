#ifndef __TERM_H__
#define __TERM_H__

typedef struct term_t {
    int r, c;
    int cur_x, cur_y;
    char **screen;
}term_t ;

term_t *get_term(int r, int c);
int term_write(term_t *term, const char *str);


#endif // !__TERM_H__
