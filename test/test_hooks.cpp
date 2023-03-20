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

int sum(int a, int b) {
    return a + b;
}
static_assert(std::is_same_v<decltype(&sum), int(*)(int, int)>);

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

NO_OPTIMIZE float RCMP_DETAIL_CDECL g1(int a, float b) {
    return a * b;
}

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

    constexpr auto converted_sum = rcmp::with_signature<sum, rcmp::cdecl_t<int(int, int)>>;
    REQUIRE(converted_sum(1, 2) == sum(1, 2));
    STATIC_REQUIRE(std::is_same_v<decltype(converted_sum), int(RCMP_DETAIL_CDECL* const)(int, int)>);
}

#endif // RCMP_HAS_CDECL()

#if RCMP_HAS_STDCALL()

NO_OPTIMIZE float RCMP_DETAIL_STDCALL g2(int a, float b) {
    return a * b;
}

static_assert(std::is_same_v<decltype(&g2), float(RCMP_DETAIL_STDCALL*)(int, float)>);
static_assert(std::is_same_v<rcmp::to_generic_signature<decltype(&g2)>, rcmp::stdcall_t<float(int, float)>>);

TEST_CASE("stdcall calling convention") {
    REQUIRE(g2(10, 10) == 100);

    rcmp::hook_function<rcmp::stdcall_t<float(int, float)>>(follow_jmp(g2), [](auto original, int a, float b) {
        return 2.0f * original(a + 1, b - 1.0f);
    });

    REQUIRE(g2(10, 10) == 198);

    constexpr auto converted_sum = rcmp::with_signature<sum, rcmp::stdcall_t<int(int, int)>>;
    REQUIRE(converted_sum(1, 2) == sum(1, 2));
    STATIC_REQUIRE(std::is_same_v<decltype(converted_sum), int(RCMP_DETAIL_STDCALL* const)(int, int)>);
}

#endif // RCMP_HAS_STDCALL()

#if RCMP_HAS_THISCALL()

struct C {
    NO_OPTIMIZE static float RCMP_DETAIL_THISCALL g3(int a, float b) {
        return a * b + 1;
    }
};

static_assert(std::is_same_v<decltype(&C::g3), float(RCMP_DETAIL_THISCALL*)(int, float)>);
static_assert(std::is_same_v<rcmp::to_generic_signature<decltype(&C::g3)>, rcmp::thiscall_t<float(int, float)>>);

TEST_CASE("thiscall calling convention") {
    REQUIRE(C::g3(10, 10) == 101);

    rcmp::hook_function<rcmp::thiscall_t<float(int, float)>>(follow_jmp(C::g3), [](auto original, int a, float b) {
        return 2.0f * original(a + 1, b - 1.0f);
    });

    REQUIRE(C::g3(10, 10) == 200);

    constexpr auto converted_sum = rcmp::with_signature<sum, rcmp::thiscall_t<int(int, int)>>;
    REQUIRE(converted_sum(1, 2) == sum(1, 2));
    STATIC_REQUIRE(std::is_same_v<decltype(converted_sum), int(RCMP_DETAIL_THISCALL* const)(int, int)>);
}

#endif // RCMP_HAS_THISCALL()

#if RCMP_HAS_FASTCALL()

NO_OPTIMIZE float RCMP_DETAIL_FASTCALL g4(int a, float b) {
    return a * b;
}

static_assert(std::is_same_v<decltype(&g4), float(RCMP_DETAIL_FASTCALL*)(int, float)>);
static_assert(std::is_same_v<rcmp::to_generic_signature<decltype(&g4)>, rcmp::fastcall_t<float(int, float)>>);

TEST_CASE("fastcall calling convention") {
    REQUIRE(g4(10, 10) == 100);

    rcmp::hook_function<rcmp::fastcall_t<float(int, float)>>(follow_jmp(g4), [](auto original, int a, float b) {
        return 2.0f * original(a + 1, b - 1.0f);
    });

    REQUIRE(g4(10, 10) == 198);

    constexpr auto converted_sum = rcmp::with_signature<sum, rcmp::fastcall_t<int(int, int)>>;
    REQUIRE(converted_sum(1, 2) == sum(1, 2));
    STATIC_REQUIRE(std::is_same_v<decltype(converted_sum), int(RCMP_DETAIL_FASTCALL* const)(int, int)>);
}

#endif // RCMP_HAS_FASTCALL()

#if RCMP_GET_PLATFORM() == RCMP_PLATFORM_WIN
    #if RCMP_GET_ARCH() == RCMP_ARCH_X86
        constexpr auto member_function_conv = rcmp::cconv::thiscall_;
    #elif RCMP_GET_ARCH() == RCMP_ARCH_X86_64
        constexpr auto member_function_conv = rcmp::cconv::native_x64;
    #endif
#elif RCMP_GET_PLATFORM() == RCMP_PLATFORM_LINUX
    #if RCMP_GET_ARCH() == RCMP_ARCH_X86
        constexpr auto member_function_conv = rcmp::cconv::cdecl_;
    #elif RCMP_GET_ARCH() == RCMP_ARCH_X86_64
        constexpr auto member_function_conv = rcmp::cconv::native_x64;
    #endif
#endif

TEST_CASE("Pointer-to-member function") {
    struct A {
        int b;

        int f(int a) const {
            return a + b;
        }
    };

#if RCMP_GET_PLATFORM() == RCMP_PLATFORM_WIN
    struct PMF {
        rcmp::flatten_pmf_t<decltype(&A::f), member_function_conv> method;
    };
#elif RCMP_GET_PLATFORM() == RCMP_PLATFORM_LINUX
    struct PMF {
        rcmp::flatten_pmf_t<decltype(&A::f), member_function_conv> method;
        std::uintptr_t dummy;
    };
#endif

    auto A_f = rcmp::bit_cast<PMF>(&A::f).method;
    A a{ 5 };
    REQUIRE(A_f(&a, 10) == a.f(10));
}

struct AV {
    int b = 5;

    NO_OPTIMIZE
    virtual int f(int a) const {
        return a + b;
    }
};

// prevent devirtualization
NO_OPTIMIZE
int call_f(AV& av, int arg) {
    return av.f(arg);
}

TEST_CASE("vtable hooking") {
    using vtable_t = std::array<rcmp::address_t, 1>;

    struct AV2 {
        vtable_t* vtable;
        int b;
    };

    static_assert(sizeof(AV) == sizeof(AV2));

    AV av;
    REQUIRE(call_f(av, 10) == 15);

    auto hook = [](auto original, auto self, int arg) -> int {
        return 2 * original(self, arg + 1);
    };

    auto& vtable = *rcmp::bit_cast<AV2*>(&av)->vtable;
    rcmp::hook_indirect_function<rcmp::generic_signature_t<int(const AV*, int), member_function_conv>>(&vtable[0], hook);

    REQUIRE(call_f(av, 10) == 32);
}

NO_OPTIMIZE
int f3(int arg) {
    return arg;
}

TEST_CASE("double hook") {
    auto l = [](auto original, auto arg) {
        return original(arg) * 2;
    };

    auto l2 = [](auto original, auto arg) {
        return original(arg) * 2;
    };

    CHECK(f3(42) == 42);
    rcmp::hook_function<decltype(f3)>(follow_jmp(f3), l);
    CHECK(f3(42) == 84);

    // Hooking with same lambda should fail because of global state
    CHECK_THROWS_WITH(rcmp::hook_function<decltype(f3)>(follow_jmp(f3), l), Catch::Contains("Cannot install hook using same state twice"));
    CHECK(f3(42) == 84);

    // However, it should work for different lambdas because of different state
    rcmp::hook_function<decltype(f3)>(follow_jmp(f3), l2);
    CHECK(f3(42) == 168);
}

NO_OPTIMIZE
int f4(int arg) {
    return arg;
}

TEST_CASE("hook with different tags") {
    auto l = [](auto original, auto arg) {
        return original(arg) + 1;
    };

    CHECK(f4(42) == 42);

    rcmp::hook_function<class Tag1, decltype(f4)>(follow_jmp(&f4), l);
    CHECK(f4(42) == 43);

    rcmp::hook_function<class Tag2, decltype(f4)>(follow_jmp(&f4), l);
    CHECK(f4(42) == 44);
}

TEST_CASE("compile-time addresses") {
    if ([[maybe_unused]] auto always_true = []{ return true; }()) {
        return;
    }

    // Just to check compilation, should not be called
    rcmp::hook_indirect_function<0x1, void()>([](auto) {});
    rcmp::hook_function<0x1, void()>([](auto) {});
}
