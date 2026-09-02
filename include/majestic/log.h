#ifndef LOG_H
#define LOG_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

enum LogType {
#ifndef NDEBUG
    L_TRACE,
#endif
    L_VERBOSE,
    L_DEBUG,
    L_INFO,
    L_WARN,
    L_ERROR,
    L_FATAL,
};

typedef struct {
    va_list ap;
    const char *fmt;
    const char *file;
    int line;
    const char *func;
    struct tm *time;
    void *udata;
    enum LogType level;
} log_Event;

typedef void (*log_LogFn)(log_Event *ev);
typedef void (*log_LockFn)(bool lock, void *udata);

const char *log_level_string(int level);
void log_set_lock(log_LockFn fn, void *udata);
void log_set_level(int level);
void log_set_quiet(bool enable);
int log_add_callback(log_LogFn fn, void *udata);
int log_add_fp(FILE *fp);

bool init_log(const char *logfile, bool with_syslog);
void deinit_log();
void log_log(
    enum LogType level, const char *file, int line, const char *func,
    const char *fmt, ...);

#ifdef NDEBUG
#define log_t(...) (void)(__VA_ARGS__)
#else
#define log_t(...)                                                             \
    log_log(L_TRACE, FILE_BASENAME, __LINE__, __func__, __VA_ARGS__)
#endif

#define log_v(...)                                                             \
    log_log(L_VERBOSE, FILE_BASENAME, __LINE__, __func__, __VA_ARGS__)
#define log_d(...)                                                             \
    log_log(L_DEBUG, FILE_BASENAME, __LINE__, __func__, __VA_ARGS__)
#define log_i(...)                                                             \
    log_log(L_INFO, FILE_BASENAME, __LINE__, __func__, __VA_ARGS__)
#define log_w(...)                                                             \
    log_log(L_WARN, FILE_BASENAME, __LINE__, __func__, __VA_ARGS__)
#define log_e(...)                                                             \
    log_log(L_ERROR, FILE_BASENAME, __LINE__, __func__, __VA_ARGS__)
#define log_f(...)                                                             \
    log_log(L_FATAL, FILE_BASENAME, __LINE__, __func__, __VA_ARGS__)

#endif /* LOG_H */
