/* Stub log4c.h for OPIC compilation without log4c library */
#ifndef LOG4C_H
#define LOG4C_H

#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>  /* for ptrdiff_t */

/* Stub category type */
typedef struct log4c_category_s {
    char name[1];
} log4c_category_t;

/* Priority levels */
#define LOG4C_PRIORITY_FATAL   000
#define LOG4C_PRIORITY_ALERT   100
#define LOG4C_PRIORITY_CRIT    200
#define LOG4C_PRIORITY_ERROR   300
#define LOG4C_PRIORITY_WARN    400
#define LOG4C_PRIORITY_NOTICE  500
#define LOG4C_PRIORITY_INFO    600
#define LOG4C_PRIORITY_DEBUG   700
#define LOG4C_PRIORITY_TRACE   800
#define LOG4C_PRIORITY_NOTEST  900
#define LOG4C_PRIORITY_UNKNOWN 1000

/* Stub functions - no-op implementations */
static inline int log4c_init(void) { return 0; }
static inline int log4c_fini(void) { return 0; }
static inline log4c_category_t* log4c_category_get(const char* name) { 
    static log4c_category_t cat; 
    return &cat; 
}

/* Stub logging function - prints to stderr for important messages */
static inline void log4c_category_log(const log4c_category_t* cat, int priority, 
                                       const char* fmt, ...) {
    (void)cat;
    (void)priority;
    (void)fmt;
    /* No-op: suppress all logging */
}

#endif /* LOG4C_H */
