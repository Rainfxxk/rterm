#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include "util.h"

FILE *log_f = NULL;

void log_init(const char *log_file) {
    log_f = (FILE *)CHECK_PTR(fopen(log_file, "w"), "can't not open file %s", log_file);
}

void term_log(const char *format, ...) {
    if (log_f != NULL) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        // vfprintf(log_f, format, args);
        va_end(args);
    }
}

void log_close() {
    if (log_f != NULL) {
        fclose(log_f);
    }
}
