#ifndef __PTY_H__
#define __PTY_Y__

typedef struct PTY PTY;

struct PTY
{
    int master, slave;
};


PTY *open_pty();
int term_set_size(PTY *pty, int r, int c);
int spawn(PTY *pty);


#endif // !__PTY_H__
