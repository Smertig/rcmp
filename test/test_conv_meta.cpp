#include "catch2/catch.hpp"

#include <rcmp.hpp>

#define CHECK_SAME(...) STATIC_REQUIRE(std::is_same_v<__VA_ARGS__>)

TEST_CASE("Convention support") {
    using namespace rcmp;

#if RCMP_GET_ARCH() == RCMP_ARCH_X86
    std::puts("RCMP_ARCH_X86");
    STATIC_REQUIRE(is_convention_supported<cconv::cdecl_>);
    STATIC_REQUIRE(is_convention_supported<cconv::stdcall_>);
    STATIC_REQUIRE(is_convention_supported<cconv::thiscall_>);
    STATIC_REQUIRE(is_convention_supported<cconv::fastcall_>);
    STATIC_REQUIRE(!is_convention_supported<cconv::native_x64>);
#elif RCMP_GET_ARCH() == RCMP_ARCH_X86_64
    std::puts("RCMP_ARCH_X86_64");
    STATIC_REQUIRE(!is_convention_supported<cconv::cdecl_>);
    STATIC_REQUIRE(!is_convention_supported<cconv::stdcall_>);
    STATIC_REQUIRE(!is_convention_supported<cconv::thiscall_>);
    STATIC_REQUIRE(!is_convention_supported<cconv::fastcall_>);
    STATIC_REQUIRE(is_convention_supported<cconv::native_x64>);
#else
    #error Unknown arch
#endif
}

TEMPLATE_TEST_CASE("Convert to generic signature", "", int, float, struct A) {
    using namespace rcmp;

#if RCMP_GET_ARCH() == RCMP_ARCH_X86 && RCMP_GET_COMPILER() == RCMP_COMPILER_MSVC
    CHECK_SAME(to_generic_signature<void(TestType)>,              generic_signature_t<void(TestType), cconv::cdecl_>);
    CHECK_SAME(to_generic_signature<void(*)(TestType)>,           generic_signature_t<void(TestType), cconv::cdecl_>);
    CHECK_SAME(to_generic_signature<void(__cdecl*)(TestType)>,    generic_signature_t<void(TestType), cconv::cdecl_>);
    CHECK_SAME(to_generic_signature<void(__stdcall*)(TestType)>,  generic_signature_t<void(TestType), cconv::stdcall_>);
    CHECK_SAME(to_generic_signature<void(__thiscall*)(TestType)>, generic_signature_t<void(TestType), cconv::thiscall_>);
    CHECK_SAME(to_generic_signature<void(__fastcall*)(TestType)>, generic_signature_t<void(TestType), cconv::fastcall_>);
#elif RCMP_GET_ARCH() == RCMP_ARCH_X86 && (RCMP_GET_COMPILER() == RCMP_COMPILER_GCC || RCMP_GET_COMPILER() == RCMP_COMPILER_CLANG)
    CHECK_SAME(to_generic_signature<void(TestType)>,                           generic_signature_t<void(TestType), cconv::cdecl_>);
    CHECK_SAME(to_generic_signature<void(*)(TestType)>,                        generic_signature_t<void(TestType), cconv::cdecl_>);
    CHECK_SAME(to_generic_signature<void(__attribute((cdecl))*)(TestType)>,    generic_signature_t<void(TestType), cconv::cdecl_>);
    CHECK_SAME(to_generic_signature<void(__attribute((stdcall))*)(TestType)>,  generic_signature_t<void(TestType), cconv::stdcall_>);
    CHECK_SAME(to_generic_signature<void(__attribute((thiscall))*)(TestType)>, generic_signature_t<void(TestType), cconv::thiscall_>);
    CHECK_SAME(to_generic_signature<void(__attribute((fastcall))*)(TestType)>, generic_signature_t<void(TestType), cconv::fastcall_>);
#elif RCMP_GET_ARCH() == RCMP_ARCH_X86_64 && RCMP_GET_COMPILER() == RCMP_COMPILER_MSVC
    CHECK_SAME(to_generic_signature<void(TestType)>,              generic_signature_t<void(TestType), cconv::native_x64>);
    CHECK_SAME(to_generic_signature<void(*)(TestType)>,           generic_signature_t<void(TestType), cconv::native_x64>);
    // TODO: forbid other calling conventions in x64 mode?
    CHECK_SAME(to_generic_signature<void(__cdecl*)(TestType)>,    generic_signature_t<void(TestType), cconv::native_x64>);
    CHECK_SAME(to_generic_signature<void(__stdcall*)(TestType)>,  generic_signature_t<void(TestType), cconv::native_x64>);
    CHECK_SAME(to_generic_signature<void(__thiscall*)(TestType)>, generic_signature_t<void(TestType), cconv::native_x64>);
    CHECK_SAME(to_generic_signature<void(__fastcall*)(TestType)>, generic_signature_t<void(TestType), cconv::native_x64>);
#elif RCMP_GET_ARCH() == RCMP_ARCH_X86_64 && (RCMP_GET_COMPILER() == RCMP_COMPILER_GCC || RCMP_GET_COMPILER() == RCMP_COMPILER_CLANG)
    CHECK_SAME(to_generic_signature<void(TestType)>,                           generic_signature_t<void(TestType), cconv::native_x64>);
    CHECK_SAME(to_generic_signature<void(*)(TestType)>,                        generic_signature_t<void(TestType), cconv::native_x64>);
    // TODO: forbid other calling conventions in x64 mode?
    CHECK_SAME(to_generic_signature<void(__attribute((cdecl))*)(TestType)>,    generic_signature_t<void(TestType), cconv::native_x64>);
    CHECK_SAME(to_generic_signature<void(__attribute((stdcall))*)(TestType)>,  generic_signature_t<void(TestType), cconv::native_x64>);
    CHECK_SAME(to_generic_signature<void(__attribute((thiscall))*)(TestType)>, generic_signature_t<void(TestType), cconv::native_x64>);
    CHECK_SAME(to_generic_signature<void(__attribute((fastcall))*)(TestType)>, generic_signature_t<void(TestType), cconv::native_x64>);
#else
    #error Unknown arch + compiler
#endif
}

TEMPLATE_TEST_CASE("Convert from generic signature", "", int, float, struct A) {
    using namespace rcmp;

#if RCMP_GET_ARCH() == RCMP_ARCH_X86 && RCMP_GET_COMPILER() == RCMP_COMPILER_MSVC
    CHECK_SAME(void(*)(TestType),           from_generic_signature<generic_signature_t<void(TestType), cconv::cdecl_>>);
    CHECK_SAME(void(__cdecl*)(TestType),    from_generic_signature<generic_signature_t<void(TestType), cconv::cdecl_>>);
    CHECK_SAME(void(__stdcall*)(TestType),  from_generic_signature<generic_signature_t<void(TestType), cconv::stdcall_>>);
    CHECK_SAME(void(__thiscall*)(TestType), from_generic_signature<generic_signature_t<void(TestType), cconv::thiscall_>>);
    CHECK_SAME(void(__fastcall*)(TestType), from_generic_signature<generic_signature_t<void(TestType), cconv::fastcall_>>);
#elif RCMP_GET_ARCH() == RCMP_ARCH_X86 && (RCMP_GET_COMPILER() == RCMP_COMPILER_GCC || RCMP_GET_COMPILER() == RCMP_COMPILER_CLANG)
    CHECK_SAME(void(*)(TestType),                        from_generic_signature<generic_signature_t<void(TestType), cconv::cdecl_>>);
    CHECK_SAME(void(__attribute((cdecl))*)(TestType),    from_generic_signature<generic_signature_t<void(TestType), cconv::cdecl_>>);
    CHECK_SAME(void(__attribute((stdcall))*)(TestType),  from_generic_signature<generic_signature_t<void(TestType), cconv::stdcall_>>);
    CHECK_SAME(void(__attribute((thiscall))*)(TestType), from_generic_signature<generic_signature_t<void(TestType), cconv::thiscall_>>);
    CHECK_SAME(void(__attribute((fastcall))*)(TestType), from_generic_signature<generic_signature_t<void(TestType), cconv::fastcall_>>);
#elif RCMP_GET_ARCH() == RCMP_ARCH_X86_64
    CHECK_SAME(void(*)(TestType), from_generic_signature<generic_signature_t<void(TestType), cconv::native_x64>>);
#else
    #error Unknown arch + compiler
#endif
}
