#ifndef __LOGGING_H__
#define __LOGGING_H__

#include "config.h"
#include <stdbool.h>
#include <stdarg.h>

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#else
enum {
	LOG_ERR,
	LOG_WARNING,
	LOG_NOTICE,
	LOG_INFO,
	LOG_DEBUG,
};
#endif

/* original / legacy debug flags */
extern bool opt_debug;
extern bool opt_log_output;
extern bool opt_realquiet;
extern bool want_per_device_stats;

/* global log_level, messages with lower or equal prio are logged */
extern int opt_log_level;

/* low-level logging functions with priority parameter */
extern void vapplog(int prio, const char *fmt, va_list ap);
extern void _applog(int prio, const char *fmt, ...);
#define applog(prio, fmt, ...) do { \
 char *tmp42; \
 if (0) \
	sprintf(tmp42, fmt, ##__VA_ARGS__); \
 else \
	_applog(prio, fmt, ##__VA_ARGS__); \
} while (0)

/* high-level logging functions with implicit priority */
extern void log_error(const char *fmt, ...);
extern void log_warning(const char *fmt, ...);
extern void log_notice(const char *fmt, ...);
extern void log_info(const char *fmt, ...);
extern void log_debug(const char *fmt, ...);

#endif /* __LOGGING_H__ */
