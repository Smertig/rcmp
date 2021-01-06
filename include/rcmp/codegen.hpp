#pragma once

#include "low_level.hpp"
#include "memory.hpp"
#include "detail/exception.hpp"
#include "detail/address.hpp"

#include <type_traits>
#include <optional>
#include <cassert>
#include <utility>

namespace rcmp {

// returns relocated original function address
rcmp::address_t make_raw_hook(rcmp::address_t original_function, rcmp::address_t wrapper_function);

namespace detail {

template <bool IsIndirect, class GenericSignature, class Hook, class... Tags>
struct hook_impl;

template <bool IsIndirect, class Ret, class... Args, cconv Convention, class Hook, class... Tags>
struct hook_impl<IsIndirect, generic_signature_t<Ret(Args...), Convention>, Hook, Tags...> {
    using generic_sig_t  = generic_signature_t<Ret(Args...), Convention>;
    using original_sig_t = rcmp::from_generic_signature<generic_sig_t>;
    using hook_t         = Hook;

    static_assert(std::is_invocable_r_v<Ret, hook_t, original_sig_t, Args...>);

    inline static std::optional<hook_t> m_hook     = std::nullopt;
    inline static original_sig_t        m_original = nullptr;

    static Ret call_hook_impl(Args... args) {
        assert(m_hook != std::nullopt);
        assert(m_original != nullptr);
        return (*m_hook)(m_original, std::forward<Args>(args)...);
    }

    static inline constexpr original_sig_t call_hook = with_signature<call_hook_impl, generic_sig_t>;

    static void do_hook(rcmp::address_t address, hook_t hook) {
        if (m_original != nullptr) {
            throw rcmp::error("double hook of %" PRIXPTR, address.as_number());
        }

        m_hook.emplace(std::move(hook));

        if constexpr (IsIndirect) {
            // `address` is an address of memory region (`sizeof(void*)` bytes) storing address of function body
            auto& function_address_ref = *rcmp::bit_cast<rcmp::address_t*>(address);

            rcmp::unprotect_memory(&function_address_ref, sizeof(function_address_ref));
            m_original = std::exchange(function_address_ref, rcmp::bit_cast<void*>(call_hook)).template as<original_sig_t>();
        }
        else {
            // `address` is an address of function body
            m_original = make_raw_hook(address, rcmp::bit_cast<void*>(call_hook)).template as<original_sig_t>();
        }
    }
};

} // namespace detail

template <auto FunctionAddress, class Signature, class F>
void hook_function(F&& hook) {
    static_assert(std::is_constructible_v<rcmp::address_t, decltype(FunctionAddress)>);

    detail::hook_impl<false, to_generic_signature<Signature>, std::decay_t<F>,
        std::integral_constant<decltype(FunctionAddress), FunctionAddress>
    >::do_hook(FunctionAddress, std::forward<F>(hook));
}

template <auto Function, class F>
void hook_function(F&& hook) {
    using Signature = decltype(Function);

    static_assert(std::is_pointer_v<Signature>,                         "OriginalFunction is not a pointer to function. Did you forget to specify signature? (rcmp::hook_function<.., Signature>(..) overload)");
    static_assert(std::is_function_v<std::remove_pointer_t<Signature>>, "OriginalFunction is not a pointer to function. Did you forget to specify signature? (rcmp::hook_function<.., Signature>(..) overload)");

    detail::hook_impl<false, to_generic_signature<Signature>, std::decay_t<F>,
        std::integral_constant<Signature, Function>
    >::do_hook(rcmp::bit_cast<const void*>(Function), std::forward<F>(hook));
}

template <class Tag, class Signature, class F>
void hook_function(rcmp::address_t function_address, F&& hook) {
    detail::hook_impl<false, to_generic_signature<Signature>, std::decay_t<F>,
        Tag
    >::do_hook(function_address, std::forward<F>(hook));
}

template <class Signature, class F>
void hook_function(rcmp::address_t function_address, F&& hook) {
    hook_function<class Tag, Signature>(function_address, std::forward<F>(hook));
}

template <auto IndirectFunctionAddress, class Signature, class F>
void hook_indirect_function(F&& hook) {
    static_assert(std::is_constructible_v<rcmp::address_t, decltype(IndirectFunctionAddress)>);

    detail::hook_impl<true, to_generic_signature<Signature>, std::decay_t<F>,
        std::integral_constant<decltype(IndirectFunctionAddress), IndirectFunctionAddress>
    >::do_hook(IndirectFunctionAddress, std::forward<F>(hook));
}

template <class Tag, class Signature, class F>
void hook_indirect_function(rcmp::address_t indirect_function_address, F&& hook) {
    detail::hook_impl<true, to_generic_signature<Signature>, std::decay_t<F>,
        Tag
    >::do_hook(indirect_function_address, std::forward<F>(hook));
}

template <class Signature, class F>
void hook_indirect_function(rcmp::address_t indirect_function_address, F&& hook) {
    hook_indirect_function<class Tag, Signature>(indirect_function_address, std::forward<F>(hook));
}

} // namespace rcmp
