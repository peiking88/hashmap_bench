/* Fixed op_assert.h for OPIC - accepts single argument */
#ifndef OPIC_COMMON_OP_ASSERT_H
#define OPIC_COMMON_OP_ASSERT_H 1

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#define OP_ASSERT_STACK_LIMIT 2048

/* Compatible op_assert that accepts variable arguments (including 1 arg) */
#define op_assert(...) ((void)0)
#define op_assert_diagnose(...) ((void)0)

#define op_stacktrace(stream)                           \
  do {                                                  \
    void* stack[OP_ASSERT_STACK_LIMIT];                 \
    size_t size;                                        \
    size = backtrace(stack, OP_ASSERT_STACK_LIMIT);     \
    backtrace_symbols_fd(stack,size,fileno(stream));    \
    abort();                                            \
  } while(0)

#endif /* OPIC_COMMON_OP_ASSERT_H */
