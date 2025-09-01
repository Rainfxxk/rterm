#define _XOPEN_SOURCE 600
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include "pty.h"
#include "util.h"

#define SHELL "/bin/bash"


int term_set_size(PTY *pty, int r, int c) {
    struct winsize ws = {
        .ws_col = c,
        .ws_row = r,
    };

    if (ioctl(pty->master, TIOCSWINSZ, &ws) == -1)
    {
        perror("ioctl(TIOCSWINSZ)");
        exit(1);
    }

    return 0;
}

PTY *open_pty(){
    PTY *pty = (PTY *)malloc(sizeof(PTY));
    char *slave_name;

    pty->master = posix_openpt(O_RDWR | O_NOCTTY);
    if (pty->master == -1)
    {
        perror("posix_openpt");
        exit(1);
    }

    if (grantpt(pty->master) == -1)
    {
        perror("grantpt");
        exit(1);
    }

    if (unlockpt(pty->master) == -1)
    {
        perror("grantpt");
        exit(1);
    }

    /* Up until now, we only have the master FD. We also need a file
     * descriptor for our child process. We get it by asking for the
     * actual path in /dev/pts which we then open using a regular
     * open(). So, unlike pipe(), you don't get two corresponding file
     * descriptors in one go. */

    slave_name = ptsname(pty->master);
    if (slave_name == NULL)
    {
        perror("ptsname");
        exit(1);
    }

    pty->slave = open(slave_name, O_RDWR | O_NOCTTY);
    if (pty->slave == -1)
    {
        perror("open(slave_name)");
        exit(1);
    }

    return pty;
}


int spawn(PTY *pty) {
    pid_t p;
    // char *env[] = { "TERM=xterm-256color", NULL };
    char *env[] = { "TERM=dumb", NULL };

    p = fork();
    if (p == 0)
    {
        close(pty->master);

        setsid();
        if (ioctl(pty->slave, TIOCSCTTY, NULL) == -1)
        {
            perror("ioctl(TIOCSCTTY)");
            exit(1);
        }

        dup2(pty->slave, 0);
        dup2(pty->slave, 1);
        dup2(pty->slave, 2);
        close(pty->slave);

        execle(SHELL, "-" SHELL, (char *)NULL, env);
        return -1;
    }
    else if (p > 0)
    {
        close(pty->slave);
        return 0;
    }

    perror("fork");
    return -1;
}

