#pragma once

// Usage: RCMP_GET_COMPILER() == RCMP_COMPILER_*
#define RCMP_COMPILER_GCC 1
#define RCMP_COMPILER_CLANG 2
#define RCMP_COMPILER_MSVC 3

// Usage: RCMP_GET_PLATFORM() == RCMP_PLATFORM_*
#define RCMP_PLATFORM_WIN 1
#define RCMP_PLATFORM_LINUX 2

// Usage: RCMP_GET_ARCH() == RCMP_PLATFORM_*
#define RCMP_ARCH_X86 1
#define RCMP_ARCH_X86_64 2
#define RCMP_ARCH_ARM64 3

#if defined(__clang__)
    #define RCMP_GET_COMPILER() RCMP_COMPILER_CLANG
#elif defined(__GNUC__)
    #define RCMP_GET_COMPILER() RCMP_COMPILER_GCC
#elif defined(_MSC_VER)
    #define RCMP_GET_COMPILER() RCMP_COMPILER_MSVC
#else
    #error Unable to detect compiler
#endif

#if defined(__linux)
    #define RCMP_GET_PLATFORM() RCMP_PLATFORM_LINUX
#elif defined(_WIN32) || defined(_WIN64)
    #define RCMP_GET_PLATFORM() RCMP_PLATFORM_WIN
#else
    #error Unable to detect platform
#endif

#if RCMP_GET_PLATFORM() == RCMP_PLATFORM_WIN
    #if defined(_WIN64)
        #define RCMP_GET_ARCH() RCMP_ARCH_X86_64
    #else
        #define RCMP_GET_ARCH() RCMP_ARCH_X86
    #endif
#elif RCMP_GET_PLATFORM() == RCMP_PLATFORM_LINUX
    #if defined(__i386__)
        #define RCMP_GET_ARCH() RCMP_ARCH_X86
    #elif defined(__x86_64__)
        #define RCMP_GET_ARCH() RCMP_ARCH_X86_64
    #elif defined(__aarch64__)
        #define RCMP_GET_ARCH() RCMP_ARCH_ARM64
    #else
        #error Unable to detect architecture
    #endif
#endif
