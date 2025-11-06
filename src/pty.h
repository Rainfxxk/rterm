#ifndef __PTY_H__
#define __PTY_H__


typedef struct pty_t {
    int master, slave;
} pty_t;


pty_t *open_pty(pty_t *pty);
int pty_set_size(pty_t *pty, int r, int c);
int spawn(pty_t *pty);


#endif // !__PTY_H__
