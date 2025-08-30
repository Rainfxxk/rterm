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

#define SHELL "/bin/bash"


int term_set_size(PTY *pty, int r, int c) {
    struct winsize ws = {
        .ws_col = c,
        .ws_row = r,
    };

    /* This is the very same ioctl that normal programs use to query the
     * window size. Normal programs are actually able to do this, too,
     * but it makes little sense: Setting the size has no effect on the
     * PTY driver in the kernel (it just keeps a record of it) or the
     * terminal emulator. IIUC, all that's happening is that subsequent
     * ioctls will report the new size -- until another ioctl sets a new
     * size.
     *
     * I didn't see any response to ioctls of normal programs in any of
     * the popular terminals (XTerm, VTE, st). They are not informed by
     * the kernel when a normal program issues an ioctl like that.
     *
     * On the other hand, if we were to issue this ioctl during runtime
     * and the size actually changed, child programs would get a
     * SIGWINCH. */
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

    /* Opens the PTY master device. This is the file descriptor that
     * we're reading from and writing to in our terminal emulator.
     *
     * We're going for BSD-style management of the controlling terminal:
     * Don't try to change anything now (O_NOCTTY), we'll issue an
     * ioctl() later on. */
    pty->master = posix_openpt(O_RDWR | O_NOCTTY);
    if (pty->master == -1)
    {
        perror("posix_openpt");
        exit(1);
    }

    /* grantpt() and unlockpt() are housekeeping functions that have to
     * be called before we can open the slave FD. Refer to the manpages
     * on what they do. */
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
    char *env[] = { "TERM=dumb", NULL };

    p = fork();
    if (p == 0)
    {
        close(pty->master);

        /* Create a new session and make our terminal this process'
         * controlling terminal. The shell that we'll spawn in a second
         * will inherit the status of session leader. */
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

