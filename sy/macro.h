// 常用宏的封装
#ifndef __SY_MACRO_H__
#define __SY_MACRO_H__

#include <string.h>
#include <assert.h>
#include "log.h"
#include "util.h"

#if defined __GNUC__ || defined __llvm__
// LIKCLY 宏的封装, 告诉编译器优化,条件大概率成立
#define SY_LIKELY(x) __builtin_expect(!!(x), 1)
// LIKCLY 宏的封装, 告诉编译器优化,条件大概率不成立
#define SY_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define SY_LIKELY(x) (x)
#define SY_UNLIKELY(x) (x)
#endif

// 断言宏封装
#define SY_ASSERT(x)                                                                \
    if (SY_UNLIKELY(!(x))) {                                                        \
        SY_LOG_ERROR(SY_LOG_ROOT()) << "ASSERTION: " #x                          \
                                          << "\nbacktrace:\n"                          \
                                          << sy::BacktraceToString(100, 2, "    "); \
        assert(x);                                                                     \
    }

// 断言宏封装
#define SY_ASSERT2(x, w)                                                            \
    if (SY_UNLIKELY(!(x))) {                                                        \
        SY_LOG_ERROR(SY_LOG_ROOT()) << "ASSERTION: " #x                          \
                                          << "\n"                                      \
                                          << w                                         \
                                          << "\nbacktrace:\n"                          \
                                          << sy::BacktraceToString(100, 2, "    "); \
        assert(x);                                                                     \
    }

#endif
