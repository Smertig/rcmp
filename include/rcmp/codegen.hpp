#pragma once

#include "low_level.hpp"
#include "memory.hpp"
#include "detail/exception.hpp"
#include "detail/address.hpp"

#include <functional>
#include <cassert>

namespace rcmp {

// returns relocated original function address
rcmp::address_t make_raw_hook(rcmp::address_t original_function, rcmp::address_t wrapper_function);

namespace detail {

template <class GenericSignature, class... Tags>
struct hook_impl;

template <class Ret, class... Args, cconv Convention, class... Tags>
struct hook_impl<generic_signature_t<Ret(Args...), Convention>, Tags...> {
    using original_sig_t = rcmp::from_generic_signature<generic_signature_t<Ret(Args...), Convention>>;
    using simple_sig_t   = Ret(*)(Args...);
    using hook_ex_t      = std::function<Ret(simple_sig_t, Args...)>;

    inline static hook_ex_t       m_replaced = nullptr;
    inline static rcmp::address_t m_original = nullptr;

    static Ret call_original(Args... args) {
        return m_original.as<original_sig_t>()(args...);
    }

    static Ret call_replaced(Args... args) {
        return m_replaced(call_original, args...);
    }

#if RCMP_HAS_CDECL()
    static Ret RCMP_DETAIL_CDECL wrapper_cdecl(Args... args) { return call_replaced(args...); }
    static auto get_wrapper(std::integral_constant<cconv, cconv::cdecl_>) { return wrapper_cdecl; }
#endif

#if RCMP_HAS_STDCALL()
    static Ret RCMP_DETAIL_STDCALL wrapper_stdcall(Args... args) { return call_replaced(args...); }
    static auto get_wrapper(std::integral_constant<cconv, cconv::stdcall_>) { return wrapper_stdcall; }
#endif

#if RCMP_HAS_THISCALL()
    static Ret RCMP_DETAIL_THISCALL wrapper_thiscall(Args... args) { return call_replaced(args...); }
    static auto get_wrapper(std::integral_constant<cconv, cconv::thiscall_>) { return wrapper_thiscall; }
#endif

#if RCMP_HAS_FASTCALL()
    static Ret RCMP_DETAIL_FASTCALL wrapper_fastcall(Args... args) { return call_replaced(args...); }
    static auto get_wrapper(std::integral_constant<cconv, cconv::fastcall_>) { return wrapper_fastcall; }
#endif

#if RCMP_HAS_NATIVE_X64_CALL()
    static Ret wrapper_native_x64(Args... args) { return call_replaced(args...); }
    static auto get_wrapper(std::integral_constant<cconv, cconv::native_x64>) { return wrapper_native_x64; }
#endif

    static void do_hook(rcmp::address_t original_function, std::function<Ret(simple_sig_t, Args...)>&& hook) {
        m_replaced = std::move(hook);

        if (m_original != nullptr) {
            throw rcmp::error("double hook of %" PRIXPTR, original_function.as_number());
        }

        original_sig_t wrapper_function = get_wrapper(std::integral_constant<cconv, Convention>{});

        m_original = make_raw_hook(original_function, rcmp::bit_cast<void*>(wrapper_function));
    }
};

} // namespace detail

template <auto OriginalFunction, class Signature, class F>
void hook_function(F&& hook) {
    static_assert(std::is_constructible_v<rcmp::address_t, decltype(OriginalFunction)>);

    detail::hook_impl<to_generic_signature<Signature>,
        std::integral_constant<decltype(OriginalFunction), OriginalFunction>,
        F
    >::do_hook(OriginalFunction, std::forward<F>(hook));
}

template <auto OriginalFunction, class F>
void hook_function(F&& hook) {
    using Signature = decltype(OriginalFunction);

    static_assert(std::is_pointer_v<Signature>,                         "OriginalFunction is not a pointer to function. Did you forget to specify signature? (rcmp::hook_function<.., Signature>(..) overload)");
    static_assert(std::is_function_v<std::remove_pointer_t<Signature>>, "OriginalFunction is not a pointer to function. Did you forget to specify signature? (rcmp::hook_function<.., Signature>(..) overload)");

    detail::hook_impl<to_generic_signature<Signature>,
        std::integral_constant<Signature, OriginalFunction>,
        F
    >::do_hook(rcmp::bit_cast<const void*>(OriginalFunction), std::forward<F>(hook));
}

template <class Tag, class Signature, class F>
void hook_function(rcmp::address_t original_function, F&& hook) {
    detail::hook_impl<to_generic_signature<Signature>,
        Tag,
        F
    >::do_hook(original_function, std::forward<F>(hook));
}

template <class Signature, class F>
void hook_function(rcmp::address_t original_function, F&& hook) {
    hook_function<class Tag, Signature>(original_function, std::forward<F>(hook));
}

} // namespace rcmp
