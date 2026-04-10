#define _XOPEN_SOURCE 600
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "pty.h"
#include "util.h"

#ifdef __linux__

#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>

#define SHELL "/bin/bash"

extern char **environ;

#elif defined(_WIN32)

#include <windows.h>
#include <process.h>

#define SHELL "powershell.exe"

#elif defined(__APPLE__) && defined(__MACH__)
// 针对 macOS 平台的代码
#elif defined(__ANDROID__)
// 针对 Android 平台的代码
#elif defined(__unix__)
// 针对 Unix 系统的代码
#elif defined(_POSIX_VERSION)
// 针对符合 POSIX 标准的系统
#else
#error "未知平台"
#endif



int pty_set_size(pty_t *pty, int r, int c) {
#ifdef __linux__

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

#elif defined(_WIN32)

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SMALL_RECT windowRect = { 0, 0, c - 1, r - 1 };
    SetConsoleWindowInfo(hConsole, TRUE, &windowRect);

#elif defined(__APPLE__) && defined(__MACH__)
// 针对 macOS 平台的代码
#elif defined(__ANDROID__)
// 针对 Android 平台的代码
#elif defined(__unix__)
// 针对 Unix 系统的代码
#elif defined(_POSIX_VERSION)
// 针对符合 POSIX 标准的系统
#else
#error "未知平台"
#endif
}

pty_t *open_pty(pty_t *pty){
#ifdef __linux__
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
#elif defined(_WIN32)
    SECURITY_ATTRIBUTES sa;

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    CHECK(CreatePipe(&pty->hOutRead, &pty->hOutWrite, &sa, 0), "failed to create pipe");
    CHECK(CreatePipe(&pty->hInRead,  &pty->hInWrite,  &sa, 0), "failed to create pipe");

    return pty;
#elif defined(__APPLE__) && defined(__MACH__)
// 针对 macOS 平台的代码
#elif defined(__ANDROID__)
// 针对 Android 平台的代码
#elif defined(__unix__)
// 针对 Unix 系统的代码
#elif defined(_POSIX_VERSION)
// 针对符合 POSIX 标准的系统
#else
#error "未知平台"
#endif
}


int spawn(pty_t *pty) {
#ifdef __linux__
    pid_t p;
    char **env = environ;

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
#elif defined(_WIN32)

    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    si.hStdOutput = pty->hOutWrite;
    si.hStdError = pty->hOutWrite;
    si.hStdInput = pty->hInRead;
    si.dwFlags = STARTF_USESTDHANDLES;

    CHECK(CreateProcess(NULL, SHELL, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi),
          "failed to create shell process");

    CloseHandle(pty->hOutWrite);
    CloseHandle(pty->hInRead);

    return 0;
#elif defined(__APPLE__) && defined(__MACH__)
// 针对 macOS 平台的代码
#elif defined(__ANDROID__)
// 针对 Android 平台的代码
#elif defined(__unix__)
// 针对 Unix 系统的代码
#elif defined(_POSIX_VERSION)
// 针对符合 POSIX 标准的系统
#else
#error "未知平台"
#endif
}

