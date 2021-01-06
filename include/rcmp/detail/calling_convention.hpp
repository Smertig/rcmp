#pragma once

#include "config.hpp"

namespace rcmp {

enum class cconv {
    cdecl_,
    stdcall_,
    thiscall_,
    fastcall_,
    native_x64
};

template <cconv Conv>
constexpr bool is_convention_supported = false;

template <class Signature, cconv Convention>
struct generic_signature_t {};

namespace detail {
    template <class Signature, cconv Convention>
    struct to_generic_signature_impl_base {
        using type = generic_signature_t<Signature, Convention>;
    };

    template <class Signature>
    struct from_generic_signature_impl_base {
        using type = Signature;
    };

    template <class GenericSignature>
    struct from_generic_signature_impl {
        static_assert(sizeof(GenericSignature) == 0, "Unable to convert GenericSignature to signature");
    };

    template <class Signature, class = void>
    struct to_generic_signature_impl {
        static_assert(sizeof(Signature) == 0, "Unable to convert Signature to generic signature");
    };

    template <auto Function, class Signature>
    struct with_signature_impl {
        static_assert(sizeof(Signature) == 0, "Unable to replace function signature");
    };

    // Simple case without pointer
    template <class Ret, class... Args>
    struct to_generic_signature_impl<Ret(Args...)> : to_generic_signature_impl<Ret(*)(Args...)> {};

    // No-op for generic signatures
    template <class Signature, cconv Convention>
    struct to_generic_signature_impl<generic_signature_t<Signature, Convention>> {
        using type = generic_signature_t<Signature, Convention>;
    };

#if RCMP_GET_ARCH() == RCMP_ARCH_X86
    #define RCMP_HAS_CDECL() 1
    #define RCMP_HAS_STDCALL() 1
    #define RCMP_HAS_THISCALL() 1
    #define RCMP_HAS_FASTCALL() 1
    #define RCMP_HAS_NATIVE_X64_CALL() 0

    #if RCMP_GET_COMPILER() == RCMP_COMPILER_MSVC
        #define RCMP_DETAIL_CDECL __cdecl
        #define RCMP_DETAIL_STDCALL __stdcall
        #define RCMP_DETAIL_THISCALL __thiscall
        #define RCMP_DETAIL_FASTCALL __fastcall
    #elif RCMP_GET_COMPILER() == RCMP_COMPILER_GCC || RCMP_GET_COMPILER() == RCMP_COMPILER_CLANG
        #define RCMP_DETAIL_CDECL __attribute((cdecl))
        #define RCMP_DETAIL_STDCALL __attribute((stdcall))
        #define RCMP_DETAIL_THISCALL __attribute((thiscall))
        #define RCMP_DETAIL_FASTCALL __attribute((fastcall))
    #else
        // TODO: soft error?
        #error Unknown compiler
    #endif

    #if RCMP_GET_COMPILER() == RCMP_COMPILER_GCC
        #if __GNUC__ == 10 && __GNUC_MINOR__ == 1 && __GNUC_PATCHLEVEL__ == 0
            #error gcc-10.1 does not work well with template specialization by function pointer that has custom calling-convention
        #endif
    #endif

    template <class Ret, class... Args>
    struct to_generic_signature_impl<Ret(RCMP_DETAIL_CDECL*)(Args...)> : to_generic_signature_impl_base<Ret(Args...), cconv::cdecl_> {};

    template <class Ret, class... Args>
    struct from_generic_signature_impl<generic_signature_t<Ret(Args...), cconv::cdecl_>> : from_generic_signature_impl_base<Ret(RCMP_DETAIL_CDECL*)(Args...)> {};

    template <auto Function, class Ret, class... Args>
    struct with_signature_impl<Function, generic_signature_t<Ret(Args...), cconv::cdecl_>> {
        static Ret RCMP_DETAIL_CDECL wrapper(Args... args) {
            return Function(static_cast<Args&&>(args)...);
        }
    };

    template <class Ret, class... Args>
    struct to_generic_signature_impl<Ret(RCMP_DETAIL_STDCALL*)(Args...)> : to_generic_signature_impl_base<Ret(Args...), cconv::stdcall_> {};

    template <class Ret, class... Args>
    struct from_generic_signature_impl<generic_signature_t<Ret(Args...), cconv::stdcall_>> : from_generic_signature_impl_base<Ret(RCMP_DETAIL_STDCALL*)(Args...)> {};

    template <auto Function, class Ret, class... Args>
    struct with_signature_impl<Function, generic_signature_t<Ret(Args...), cconv::stdcall_>> {
        static Ret RCMP_DETAIL_STDCALL wrapper(Args... args) {
            return Function(static_cast<Args&&>(args)...);
        }
    };

    // warning: ‘thiscall’ attribute is used for non-class method
    #if RCMP_GET_COMPILER() == RCMP_COMPILER_GCC
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wattributes"
    #endif

    template <class Ret, class... Args>
    struct to_generic_signature_impl<Ret(RCMP_DETAIL_THISCALL*)(Args...)> : to_generic_signature_impl_base<Ret(Args...), cconv::thiscall_> {};

    template <class Ret, class... Args>
    struct from_generic_signature_impl<generic_signature_t<Ret(Args...), cconv::thiscall_>> : from_generic_signature_impl_base<Ret(RCMP_DETAIL_THISCALL*)(Args...)> {};

    template <auto Function, class Ret, class... Args>
    struct with_signature_impl<Function, generic_signature_t<Ret(Args...), cconv::thiscall_>> {
        static Ret RCMP_DETAIL_THISCALL wrapper(Args... args) {
            return Function(static_cast<Args&&>(args)...);
        }
    };

    #if RCMP_GET_COMPILER() == RCMP_COMPILER_GCC
        #pragma GCC diagnostic pop
    #endif

    template <class Ret, class... Args>
    struct to_generic_signature_impl<Ret(RCMP_DETAIL_FASTCALL*)(Args...)> : to_generic_signature_impl_base<Ret(Args...), cconv::fastcall_> {};

    template <class Ret, class... Args>
    struct from_generic_signature_impl<generic_signature_t<Ret(Args...), cconv::fastcall_>> : from_generic_signature_impl_base<Ret(RCMP_DETAIL_FASTCALL*)(Args...)> {};

    template <auto Function, class Ret, class... Args>
    struct with_signature_impl<Function, generic_signature_t<Ret(Args...), cconv::fastcall_>> {
        static Ret RCMP_DETAIL_FASTCALL wrapper(Args... args) {
            return Function(static_cast<Args&&>(args)...);
        }
    };
#else
    #define RCMP_HAS_CDECL() 0
    #define RCMP_HAS_STDCALL() 0
    #define RCMP_HAS_THISCALL() 0
    #define RCMP_HAS_FASTCALL() 0
    #define RCMP_HAS_NATIVE_X64_CALL() 1

    template <class Ret, class... Args>
    struct to_generic_signature_impl<Ret(*)(Args...)> : to_generic_signature_impl_base<Ret(Args...), cconv::native_x64> {};

    template <class Ret, class... Args>
    struct from_generic_signature_impl<generic_signature_t<Ret(Args...), cconv::native_x64>> : from_generic_signature_impl_base<Ret(*)(Args...)> {};

    template <auto Function, class Ret, class... Args>
    struct with_signature_impl<Function, generic_signature_t<Ret(Args...), cconv::native_x64>> {
        static Ret wrapper(Args... args) {
            return Function(static_cast<Args&&>(args)...);
        }
    };
#endif
} // namespace detail

template <class Signature>
using to_generic_signature = typename detail::to_generic_signature_impl<Signature>::type;

template <class GenericSignature>
using from_generic_signature = typename detail::from_generic_signature_impl<GenericSignature>::type;

template <auto Function, class Signature>
constexpr auto with_signature = detail::with_signature_impl<Function, to_generic_signature<Signature>>::wrapper;

#if RCMP_HAS_CDECL()
    template <> inline constexpr bool is_convention_supported<cconv::cdecl_> = true;

    template <class Signature>
    using cdecl_t = generic_signature_t<Signature, cconv::cdecl_>;
#endif

#if RCMP_HAS_STDCALL()
    template <> inline constexpr bool is_convention_supported<cconv::stdcall_> = true;

    template <class Signature>
    using stdcall_t = generic_signature_t<Signature, cconv::stdcall_>;
#endif

#if RCMP_HAS_THISCALL()
    template <> inline constexpr bool is_convention_supported<cconv::thiscall_> = true;

    template <class Signature>
    using thiscall_t = generic_signature_t<Signature, cconv::thiscall_>;
#endif

#if RCMP_HAS_FASTCALL()
    template <> inline constexpr bool is_convention_supported<cconv::fastcall_> = true;

    template <class Signature>
    using fastcall_t = generic_signature_t<Signature, cconv::fastcall_>;
#endif

#if RCMP_HAS_NATIVE_X64_CALL()
    template <> inline constexpr bool is_convention_supported<cconv::native_x64> = true;
#endif

} // namespace rcmp
