#ifndef __PTY_H__
#define __PTY_H__

#ifdef __linux__

#define WRITE_PTY(pty, str, len)     write(PTY_WRITE_FD(pty), str, len)
#define READ_PTY(pty, buf, buf_size) read(PTY_READ_FD(pty), buf, buf_size)

#define PTY_READ_FD(pty)  (pty.master)
#define PTY_WRITE_FD(pty) (pty.master)

#elif defined(_WIN32)

#include <windows.h>

static inline int write_handle(HANDLE fd, const void *buf, int len) {
    DWORD bytes;
    WriteFile(fd, buf, len, &bytes, NULL);
    return (int)bytes;
}

static inline int read_handle(HANDLE hnd, void *buf, int len) {
    DWORD bytes = 0;
    if (!ReadFile(hnd, buf, len, &bytes, NULL)) {
        return -1;
    }
    return (int)bytes;
}

#define WRITE_PTY(pty, str, len)     write_handle(PTY_WRITE_FD(pty), str, len)
#define READ_PTY(pty, buf, buf_size) read_handle(PTY_READ_FD(pty), buf, buf_size)

#define PTY_READ_FD(pty)  (pty.hOutRead)
#define PTY_WRITE_FD(pty) (pty.hInWrite)


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


typedef struct pty_t {
#ifdef __linux__
    int master, slave;
#elif defined(_WIN32)
    HANDLE hOutRead, hOutWrite;
    HANDLE hInRead,  hInWrite;
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
} pty_t;

pty_t *open_pty(pty_t *pty);
int pty_set_size(pty_t *pty, int r, int c);
int spawn(pty_t *pty);


#endif // !__PTY_H__
