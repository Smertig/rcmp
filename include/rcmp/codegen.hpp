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
        return (*m_hook)(m_original, std::forward<Args>(args)...);
    }

    static inline constexpr original_sig_t call_hook = with_signature<call_hook_impl, generic_sig_t>;

    template <class Policy>
    static void do_hook(rcmp::address_t address, hook_t hook) {
        if (m_original != nullptr) {
            throw rcmp::error("double hook of %" PRIXPTR, address.as_number());
        }

        m_hook.emplace(std::move(hook));
        m_original = Policy::make_raw_hook(address, rcmp::bit_cast<void*>(call_hook)).template as<original_sig_t>();
    }
};

#if RCMP_GET_ARCH() == RCMP_ARCH_X86 || RCMP_GET_ARCH() == RCMP_ARCH_X86_64

// returns relocated original function address
rcmp::address_t make_x86_x86_64_raw_hook(rcmp::address_t original_function, rcmp::address_t wrapper_function);

#define RCMP_HAS_HOOK_PROLOG_POLICY
struct HookPrologPolicy {
    static rcmp::address_t make_raw_hook(rcmp::address_t address, rcmp::address_t wrapper_function) {
        // `address` is an address of function body start
        return make_x86_x86_64_raw_hook(address, wrapper_function);
    }
};

#define RCMP_HAS_HOOK_INDIRECT_POLICY
struct HookIndirectPolicy {
    static rcmp::address_t make_raw_hook(rcmp::address_t address, rcmp::address_t wrapper_function) {
        // `address` is an address of memory region (`sizeof(void*)` bytes) storing address of function body
        auto& function_address_ref = *address.as_ptr<rcmp::address_t>();

        rcmp::unprotect_memory(&function_address_ref, sizeof(function_address_ref));
        return std::exchange(function_address_ref, wrapper_function);
    }
};

#endif

} // namespace detail

#if defined(RCMP_HAS_HOOK_PROLOG_POLICY)
template <auto FunctionAddress, class Signature, class F>
void hook_function(F&& hook) {
    static_assert(std::is_constructible_v<rcmp::address_t, decltype(FunctionAddress)>);

    detail::hook_impl<to_generic_signature<Signature>, std::decay_t<F>,
        std::integral_constant<decltype(FunctionAddress), FunctionAddress>
    >::template do_hook<detail::HookPrologPolicy>(FunctionAddress, std::forward<F>(hook));
}

template <auto Function, class F>
void hook_function(F&& hook) {
    using Signature = decltype(Function);

    static_assert(std::is_pointer_v<Signature>,                         "OriginalFunction is not a pointer to function. Did you forget to specify signature? (rcmp::hook_function<.., Signature>(..) overload)");
    static_assert(std::is_function_v<std::remove_pointer_t<Signature>>, "OriginalFunction is not a pointer to function. Did you forget to specify signature? (rcmp::hook_function<.., Signature>(..) overload)");

    detail::hook_impl<to_generic_signature<Signature>, std::decay_t<F>,
        std::integral_constant<Signature, Function>
    >::template do_hook<detail::HookPrologPolicy>(rcmp::bit_cast<const void*>(Function), std::forward<F>(hook));
}

template <class Tag, class Signature, class F>
void hook_function(rcmp::address_t function_address, F&& hook) {
    detail::hook_impl<to_generic_signature<Signature>, std::decay_t<F>,
        Tag
    >::template do_hook<detail::HookPrologPolicy>(function_address, std::forward<F>(hook));
}

template <class Signature, class F>
void hook_function(rcmp::address_t function_address, F&& hook) {
    hook_function<class Tag, Signature>(function_address, std::forward<F>(hook));
}
#endif

#if defined(RCMP_HAS_HOOK_INDIRECT_POLICY)
template <auto IndirectFunctionAddress, class Signature, class F>
void hook_indirect_function(F&& hook) {
    static_assert(std::is_constructible_v<rcmp::address_t, decltype(IndirectFunctionAddress)>);

    detail::hook_impl<to_generic_signature<Signature>, std::decay_t<F>,
        std::integral_constant<decltype(IndirectFunctionAddress), IndirectFunctionAddress>
    >::template do_hook<detail::HookIndirectPolicy>(IndirectFunctionAddress, std::forward<F>(hook));
}

template <class Tag, class Signature, class F>
void hook_indirect_function(rcmp::address_t indirect_function_address, F&& hook) {
    detail::hook_impl<to_generic_signature<Signature>, std::decay_t<F>,
        Tag
    >::template do_hook<detail::HookIndirectPolicy>(indirect_function_address, std::forward<F>(hook));
}

template <class Signature, class F>
void hook_indirect_function(rcmp::address_t indirect_function_address, F&& hook) {
    hook_indirect_function<class Tag, Signature>(indirect_function_address, std::forward<F>(hook));
}
#endif

} // namespace rcmp
