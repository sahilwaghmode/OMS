#ifndef BRANCH_PREDICTION_H
#define BRANCH_PREDICTION_H
#include <cstring>
#include <iostream>

// Cross-platform branch prediction macros
// These help the compiler optimize for expected code paths

#if defined(__GNUC__) || defined(__clang__)
// GCC and Clang (Linux, macOS, and some Windows builds)
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#elif defined(_MSC_VER)
// Microsoft Visual C++ (Windows)
// MSVC doesn't have __builtin_expect, but has its own intrinsics
// Note: These are available in newer versions of MSVC
#if _MSC_VER >= 1920    // Visual Studio 2019 and later
#define LIKELY(x) (x)   // MSVC has profile-guided optimization instead
#define UNLIKELY(x) (x) // You can use [[likely]] and [[unlikely]] in C++20
#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif
#else
// Fallback for other compilers
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif

// Alternative version using C++20 attributes (if available)
#ifdef __cplusplus
#if __cplusplus >= 202002L // C++20
#undef LIKELY
#undef UNLIKELY
#define LIKELY(x) (x) [[likely]]
#define UNLIKELY(x) (x) [[unlikely]]
#endif
#endif

inline auto ASSERT(bool cond, const std::string &msg) noexcept {
  if (UNLIKELY(!cond)) {
    std::cerr << "ASSERT : " << msg << std::endl;

    exit(EXIT_FAILURE);
  }
}

inline auto FATAL(const std::string &msg) noexcept {
  std::cerr << "FATAL : " << msg << std::endl;

  exit(EXIT_FAILURE);
}

#endif // BRANCH_PREDICTION_H

/*
Usage examples:

if (LIKELY(ptr != nullptr)) {
    // This branch is expected to be taken most of the time
    return ptr->value;
}

if (UNLIKELY(error_occurred)) {
    // This branch is expected to be taken rarely
    handle_error();
    return -1;
}

// The macros help the compiler:
// 1. Arrange code layout for better cache performance
// 2. Generate more efficient assembly
// 3. Improve branch prediction in the CPU
*/