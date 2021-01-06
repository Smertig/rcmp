#include "catch2/catch.hpp"

#include <rcmp.hpp>

#include <array>

// TODO:
//  Compiler inserts `call __x86_get_pc_thunk_ax` in function prolog, that works incorrectly after relocating.
//  However, x86-64 build is not working with -fno-PIC..

#if RCMP_GET_COMPILER() == RCMP_COMPILER_MSVC
    #define NO_OPTIMIZE __declspec(noinline)
#elif RCMP_GET_COMPILER() == RCMP_COMPILER_GCC
    #define NO_OPTIMIZE [[gnu::noinline, gnu::optimize(0)]] __attribute__((__visibility__("hidden")))
#elif RCMP_GET_COMPILER() == RCMP_COMPILER_CLANG
    #define NO_OPTIMIZE [[gnu::noinline, clang::optnone]]
#else
    #error Unknown compiler
#endif

// TODO: remove 'if' when jmp relocation is ready
#if RCMP_GET_ARCH() == RCMP_ARCH_X86

NO_OPTIMIZE int bar(float arg) {
    return static_cast<int>(arg) + 5;
}

TEST_CASE("Simplest hook") {
    REQUIRE(bar(5.1f) == 10);

    rcmp::hook_function<&bar>([](auto original_bar, float arg) {
        return original_bar(2 * arg) + 1;
    });

    REQUIRE(bar(5.1f) == 16);
}

#endif // RCMP_GET_ARCH() == RCMP_ARCH_X86

template <class Signature>
rcmp::address_t follow_jmp(Signature function) {
    rcmp::address_t address = rcmp::bit_cast<const void*>(function);

#ifdef _WIN64
    // j_foo -> foo
    // TODO: [XX YY YY YY YY] jmp relocated incorrectly on x64
    // TODO: automatically follow jumps
    address += *(address + 1).as_ptr<std::int32_t>() + 5;
#endif
    return address;
}

NO_OPTIMIZE int foo(int arg) {
    return arg + 1;
}

TEST_CASE("Hooks") {
    rcmp::address_t foo_address = follow_jmp(foo);

    REQUIRE(foo(1) == 2);

    rcmp::hook_function<int(*)(int)>(foo_address, [](auto original, int arg) {
        return original(arg) * 2;
    });

    REQUIRE(foo(1) == 4);

    const auto hook = [](auto original, int arg) {
        return original(arg) * 2;
    };
    rcmp::hook_function<int(int)>(foo_address, hook);

    REQUIRE(foo(1) == 8);

#if RCMP_HAS_NATIVE_X64_CALL()
    rcmp::hook_function<rcmp::generic_signature_t<int(int), rcmp::cconv::native_x64>>(foo_address, [](auto original, int arg) {
        return original(arg) * 3;
    });

    REQUIRE(foo(1) == 24);
#endif
}

#if RCMP_HAS_CDECL()

#if defined(_MSC_VER)

    NO_OPTIMIZE float __cdecl g1(int a, float b) {
        return a * b;
    }

#else // defined(_MSC_VER)

    [[gnu::cdecl]]
    NO_OPTIMIZE float g1(int a, float b) {
        return a * b;
    }

#endif

static_assert(std::is_same_v<decltype(&g1), float(RCMP_DETAIL_CDECL*)(int, float)>);
static_assert(std::is_same_v<rcmp::to_generic_signature<decltype(&g1)>, rcmp::cdecl_t<float(int, float)>>);

TEST_CASE("cdecl calling convention") {
    REQUIRE(g1(10, 10.0f) == Approx(100.0f));

    rcmp::hook_function<rcmp::cdecl_t<float(int, float)>>(follow_jmp(g1), [](auto original, int a, float b) {
        REQUIRE(a == 10);
        REQUIRE(b == Approx(10.0f));
        return 2.0f * original(a + 1, b - 1.0f);
    });

    REQUIRE(g1(10, 10.0f) == Approx(198.0f));

    constexpr auto sum = +[](int a, int b) { return a + b; };
    constexpr auto converted_sum = rcmp::with_signature<sum, rcmp::cdecl_t<int(int, int)>>;
    REQUIRE(converted_sum(1, 2) == sum(1, 2));
    STATIC_REQUIRE(std::is_same_v<decltype(sum), int(* const)(int, int)>);
    STATIC_REQUIRE(std::is_same_v<decltype(converted_sum), int(RCMP_DETAIL_CDECL* const)(int, int)>);
}

#endif // RCMP_HAS_CDECL()

#if RCMP_HAS_STDCALL()

#if defined(_MSC_VER)

    NO_OPTIMIZE float __stdcall g2(int a, float b) {
        return a * b;
    }

#else // defined(_MSC_VER)

    [[gnu::stdcall]]
    NO_OPTIMIZE float g2(int a, float b) {
        return a * b;
    }

#endif

static_assert(std::is_same_v<decltype(&g2), float(RCMP_DETAIL_STDCALL*)(int, float)>);
static_assert(std::is_same_v<rcmp::to_generic_signature<decltype(&g2)>, rcmp::stdcall_t<float(int, float)>>);

TEST_CASE("stdcall calling convention") {
    REQUIRE(g2(10, 10) == 100);

    rcmp::hook_function<rcmp::stdcall_t<float(int, float)>>(follow_jmp(g2), [](auto original, int a, float b) {
        return 2.0f * original(a + 1, b - 1.0f);
    });

    REQUIRE(g2(10, 10) == 198);

    constexpr auto sum = +[](int a, int b) { return a + b; };
    constexpr auto converted_sum = rcmp::with_signature<sum, rcmp::stdcall_t<int(int, int)>>;
    REQUIRE(converted_sum(1, 2) == sum(1, 2));
    STATIC_REQUIRE(std::is_same_v<decltype(sum), int(* const)(int, int)>);
    STATIC_REQUIRE(std::is_same_v<decltype(converted_sum), int(RCMP_DETAIL_STDCALL* const)(int, int)>);
}

#endif // RCMP_HAS_STDCALL()

#if RCMP_HAS_THISCALL() && !defined(_MSC_VER)

[[gnu::thiscall]]
NO_OPTIMIZE float g3(int a, float b) {
    return a * b;
}

static_assert(std::is_same_v<decltype(&g3), float(RCMP_DETAIL_THISCALL*)(int, float)>);
static_assert(std::is_same_v<rcmp::to_generic_signature<decltype(&g3)>, rcmp::thiscall_t<float(int, float)>>);

TEST_CASE("thiscall calling convention") {
    REQUIRE(g3(10, 10) == 100);

    rcmp::hook_function<rcmp::thiscall_t<float(int, float)>>(follow_jmp(g3), [](auto original, int a, float b) {
        return 2.0f * original(a + 1, b - 1.0f);
    });

    REQUIRE(g3(10, 10) == 198);

    constexpr auto sum = +[](int a, int b) { return a + b; };
    constexpr auto converted_sum = rcmp::with_signature<sum, rcmp::thiscall_t<int(int, int)>>;
    REQUIRE(converted_sum(1, 2) == sum(1, 2));
    STATIC_REQUIRE(std::is_same_v<decltype(sum), int(* const)(int, int)>);
    STATIC_REQUIRE(std::is_same_v<decltype(converted_sum), int(RCMP_DETAIL_THISCALL* const)(int, int)>);
}

#endif // RCMP_HAS_THISCALL()

#if RCMP_HAS_FASTCALL()

#if defined(_MSC_VER)

    NO_OPTIMIZE float __fastcall g4(int a, float b) {
        return a * b;
    }

#else // defined(_MSC_VER)

    [[gnu::fastcall]]
    NO_OPTIMIZE float g4(int a, float b) {
        return a * b;
    }

#endif

static_assert(std::is_same_v<decltype(&g4), float(RCMP_DETAIL_FASTCALL*)(int, float)>);
static_assert(std::is_same_v<rcmp::to_generic_signature<decltype(&g4)>, rcmp::fastcall_t<float(int, float)>>);

TEST_CASE("fastcall calling convention") {
    REQUIRE(g4(10, 10) == 100);

    rcmp::hook_function<rcmp::fastcall_t<float(int, float)>>(follow_jmp(g4), [](auto original, int a, float b) {
        return 2.0f * original(a + 1, b - 1.0f);
    });

    REQUIRE(g4(10, 10) == 198);

    constexpr auto sum = +[](int a, int b) { return a + b; };
    constexpr auto converted_sum = rcmp::with_signature<sum, rcmp::fastcall_t<int(int, int)>>;
    REQUIRE(converted_sum(1, 2) == sum(1, 2));
    STATIC_REQUIRE(std::is_same_v<decltype(sum), int(* const)(int, int)>);
    STATIC_REQUIRE(std::is_same_v<decltype(converted_sum), int(RCMP_DETAIL_FASTCALL* const)(int, int)>);
}

#endif // RCMP_HAS_FASTCALL()

TEST_CASE("Pointer-to-member function") {
    struct A {
        int b;

        int f(int a) const {
            return a + b;
        }
    };

#if RCMP_GET_PLATFORM() == RCMP_PLATFORM_WIN
    #if RCMP_GET_ARCH() == RCMP_ARCH_X86
        struct PMF {
            rcmp::flatten_pmf_t<decltype(&A::f), rcmp::cconv::thiscall_> method;
        };
    #elif RCMP_GET_ARCH() == RCMP_ARCH_X86_64
        struct PMF {
            rcmp::flatten_pmf_t<decltype(&A::f), rcmp::cconv::native_x64> method;
        };
    #endif
#elif RCMP_GET_PLATFORM() == RCMP_PLATFORM_LINUX
    #if RCMP_GET_ARCH() == RCMP_ARCH_X86
        struct PMF {
            rcmp::flatten_pmf_t<decltype(&A::f), rcmp::cconv::cdecl_> method;
            std::uint32_t dummy;
        };
    #elif RCMP_GET_ARCH() == RCMP_ARCH_X86_64
        struct PMF {
            rcmp::flatten_pmf_t<decltype(&A::f), rcmp::cconv::native_x64> method;
            std::uint64_t dummy;
        };
    #endif
#endif

    auto A_f = rcmp::bit_cast<PMF>(&A::f).method;
    A a{ 5 };
    REQUIRE(A_f(&a, 10) == a.f(10));
}
