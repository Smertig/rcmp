#pragma once

#include "with_global_state.hpp"

#include <rcmp/memory.hpp>

#include <utility>

namespace rcmp {

namespace detail {

#if RCMP_GET_ARCH() == RCMP_ARCH_X86 || RCMP_GET_ARCH() == RCMP_ARCH_X86_64

#define RCMP_HAS_HOOK_INDIRECT_POLICY

struct HookIndirectPolicy {
    static rcmp::address_t install_stateless_hook(rcmp::address_t address, rcmp::address_t wrapper_function) {
        // `address` is an address of memory region (`sizeof(void*)` bytes) storing address of function body
        auto& function_address_ref = *address.as_ptr<rcmp::address_t>();

        rcmp::unprotect_memory(&function_address_ref, sizeof(function_address_ref));
        return std::exchange(function_address_ref, wrapper_function);
    }
};

#endif

}

#if defined(RCMP_HAS_HOOK_INDIRECT_POLICY)

template <auto IndirectFunctionAddress, class Signature, class F>
void hook_indirect_function(F&& hook) {
    static_assert(std::is_constructible_v<rcmp::address_t, decltype(IndirectFunctionAddress)>);

    using wrapped_policy_t = detail::WithGlobalState<
        detail::HookIndirectPolicy,
        std::integral_constant<decltype(IndirectFunctionAddress), IndirectFunctionAddress>
    >;
    rcmp::generic_hook_function<wrapped_policy_t::template Policy, Signature>(IndirectFunctionAddress, std::forward<F>(hook));
}

template <class Tag, class Signature, class F>
void hook_indirect_function(rcmp::address_t indirect_function_address, F&& hook) {
    using wrapped_policy_t = detail::WithGlobalState<
        detail::HookIndirectPolicy,
        Tag
    >;
    rcmp::generic_hook_function<wrapped_policy_t::template Policy, Signature>(indirect_function_address, std::forward<F>(hook));
}

template <class Signature, class F>
void hook_indirect_function(rcmp::address_t indirect_function_address, F&& hook) {
    rcmp::hook_indirect_function<class Tag, Signature>(indirect_function_address, std::forward<F>(hook));
}

#endif

}
