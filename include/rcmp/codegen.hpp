#pragma once

#include "low_level.hpp"
#include "memory.hpp"
#include "detail/exception.hpp"
#include "detail/address.hpp"

#include <type_traits>
#include <optional>
#include <cassert>

namespace rcmp {

// returns relocated original function address
rcmp::address_t make_raw_hook(rcmp::address_t original_function, rcmp::address_t wrapper_function);

namespace detail {

template <class GenericSignature, class Hook, class... Tags>
struct hook_impl;

template <class Ret, class... Args, cconv Convention, class Hook, class... Tags>
struct hook_impl<generic_signature_t<Ret(Args...), Convention>, Hook, Tags...> {
    using generic_sig_t  = generic_signature_t<Ret(Args...), Convention>;
    using original_sig_t = rcmp::from_generic_signature<generic_sig_t>;
    using hook_t         = Hook;

    static_assert(std::is_invocable_r_v<Ret, hook_t, original_sig_t, Args...>);

    inline static std::optional<hook_t> m_hook     = std::nullopt;
    inline static original_sig_t        m_original = nullptr;

    static Ret call_hook_impl(Args... args) {
        assert(m_hook != std::nullopt);
        assert(m_original != nullptr);
        return (*m_hook)(m_original, args...);
    }

    static inline constexpr original_sig_t call_hook = with_signature<call_hook_impl, generic_sig_t>;

    static void do_hook(rcmp::address_t original_function, hook_t hook) {
        if (m_original != nullptr) {
            throw rcmp::error("double hook of %" PRIXPTR, original_function.as_number());
        }

        m_hook.emplace(std::move(hook));
        m_original = make_raw_hook(original_function, rcmp::bit_cast<void*>(call_hook)).template as<original_sig_t>();
    }
};

} // namespace detail

template <auto OriginalFunction, class Signature, class F>
void hook_function(F&& hook) {
    static_assert(std::is_constructible_v<rcmp::address_t, decltype(OriginalFunction)>);

    detail::hook_impl<to_generic_signature<Signature>, std::decay_t<F>,
        std::integral_constant<decltype(OriginalFunction), OriginalFunction>
    >::do_hook(OriginalFunction, std::forward<F>(hook));
}

template <auto OriginalFunction, class F>
void hook_function(F&& hook) {
    using Signature = decltype(OriginalFunction);

    static_assert(std::is_pointer_v<Signature>,                         "OriginalFunction is not a pointer to function. Did you forget to specify signature? (rcmp::hook_function<.., Signature>(..) overload)");
    static_assert(std::is_function_v<std::remove_pointer_t<Signature>>, "OriginalFunction is not a pointer to function. Did you forget to specify signature? (rcmp::hook_function<.., Signature>(..) overload)");

    detail::hook_impl<to_generic_signature<Signature>, std::decay_t<F>,
        std::integral_constant<Signature, OriginalFunction>
    >::do_hook(rcmp::bit_cast<const void*>(OriginalFunction), std::forward<F>(hook));
}

template <class Tag, class Signature, class F>
void hook_function(rcmp::address_t original_function, F&& hook) {
    detail::hook_impl<to_generic_signature<Signature>, std::decay_t<F>,
        Tag
    >::do_hook(original_function, std::forward<F>(hook));
}

template <class Signature, class F>
void hook_function(rcmp::address_t original_function, F&& hook) {
    hook_function<class Tag, Signature>(original_function, std::forward<F>(hook));
}

} // namespace rcmp
