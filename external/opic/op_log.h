/**
 * @file op_log.h
 * @brief Simplified logging header - logs disabled for benchmark
 * 
 * Original opic used log4c, but for this benchmark we disable logging
 * to focus on hashmap performance.
 */

#ifndef OPIC_COMMON_OP_LOG_H
#define OPIC_COMMON_OP_LOG_H 1

#include <opic/op_macros.h>
#include <stdio.h>

OP_BEGIN_DECLS

// Log priority levels (for compatibility)
#define LOG4C_PRIORITY_FATAL   1
#define LOG4C_PRIORITY_ALERT   2
#define LOG4C_PRIORITY_CRIT    3
#define LOG4C_PRIORITY_ERROR   4
#define LOG4C_PRIORITY_WARN    5
#define LOG4C_PRIORITY_NOTICE  6
#define LOG4C_PRIORITY_INFO    7
#define LOG4C_PRIORITY_DEBUG   8
#define LOG4C_PRIORITY_TRACE   9
#define LOG4C_PRIORITY_NOTSET  10

// Stub type
typedef void* log4c_category_t;

// Stub functions
static inline void opic_log4c_init(void) {}
static inline log4c_category_t log4c_category_get(const char* name) { 
    (void)name; 
    return NULL; 
}

OP_END_DECLS

// Logger factory - creates a stub logger
#define OP_LOGGER_FACTORY(logger, logger_namespace) \
    static log4c_category_t logger = NULL

// Logging macros - disabled for performance
#define OP_LOG_ARGS(LOGGER, CATEGORY, MESSAGE, ...)      ((void)0)
#define OP_LOG_NO_ARGS(LOGGER, CATEGORY, MESSAGE)        ((void)0)
#define OP_LOG_FATAL(LOGGER, ...)                         ((void)0)
#define OP_LOG_ALERT(LOGGER, ...)                         ((void)0)
#define OP_LOG_CRIT(LOGGER, ...)                          ((void)0)
#define OP_LOG_ERROR(LOGGER, ...)                         ((void)0)
#define OP_LOG_WARN(LOGGER, ...)                          ((void)0)
#define OP_LOG_NOTICE(LOGGER, ...)                        ((void)0)
#define OP_LOG_INFO(LOGGER, ...)                          ((void)0)
#define OP_LOG_DEBUG(LOGGER, ...)                         ((void)0)
#define OP_LOG_TRACE(LOGGER, ...)                         ((void)0)
#define OP_LOG_NOTEST(LOGGER, ...)                        ((void)0)

// Macro to get macro by args (simplified)
#define _OP_GET_MACRO_BY_ARGS(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
                              _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
                              _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
                              _31, _32, _33, _34, _35, NAME, ...) NAME

#endif
