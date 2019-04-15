#ifndef _LOGGER_C_H_
#define _LOGGER_C_H_

#ifdef _cplusplus
extern "C" {
#endif

   extern void call_Logger_debug(const char *file, int line, const char *func, const char *fmt, ...);
   extern void call_Logger_info(const char *file, int line, const char *func, const char *fmt, ...);
   extern void call_Logger_warn(const char *file, int line, const char *func, const char *fmt, ...);
   extern void call_Logger_error(const char *file, int line, const char *func, const char *fmt, ...);
   extern void call_Logger_fatal(const char *file, int line, const char *func, const char *fmt, ...);

#ifdef _cplusplus
}
#endif

#endif // _LOGGER_C_H_

