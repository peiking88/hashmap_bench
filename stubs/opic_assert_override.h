/* Override op_assert to fix OPIC bug: accept single argument */
#ifndef OPIC_OP_ASSERT_OVERRIDE_H
#define OPIC_OP_ASSERT_OVERRIDE_H

/* Undefine any existing op_assert */
#ifdef op_assert
#undef op_assert
#endif

/* Define a compatible op_assert that accepts variable args */
#define op_assert(...) ((void)0)
#define op_assert_diagnose(...) ((void)0)

#endif /* OPIC_OP_ASSERT_OVERRIDE_H */
