#pragma once

#include "with_global_state.hpp"

namespace rcmp {

namespace detail {

#if RCMP_GET_ARCH() == RCMP_ARCH_X86 || RCMP_GET_ARCH() == RCMP_ARCH_X86_64

#define RCMP_HAS_HOOK_PROLOG_POLICY

// returns relocated original function address
rcmp::address_t install_x86_x86_64_raw_hook(rcmp::address_t original_function, rcmp::address_t wrapper_function);

struct HookPrologStatelessPolicy {
    static rcmp::address_t install_stateless_hook(rcmp::address_t address, rcmp::address_t wrapper_function) {
        // `address` is an address of function body start
        return install_x86_x86_64_raw_hook(address, wrapper_function);
    }
};

#if RCMP_GET_ARCH() == RCMP_ARCH_X86
rcmp::address_t install_x86_x86_64_hook_with_tls_state(rcmp::address_t original_function, rcmp::address_t wrapper_function, void* state, void(*state_saver)(void*));

template <class HookState>
struct HookPrologTlsStatePolicy {
    static rcmp::address_t install_raw_hook(HookState* state, rcmp::address_t address, rcmp::address_t wrapper_function) {
        return install_x86_x86_64_hook_with_tls_state(address, wrapper_function, state, +[](void* current_state) {
            set_state(static_cast<HookState*>(current_state));
        });
    }

    static HookState* allocate_state([[maybe_unused]] rcmp::address_t address) {
        return new HookState;
    }

    static HookState* get_state() {
        assert(g_current_state != nullptr);
        return g_current_state;
    }

    static void set_state(HookState* state) {
        g_current_state = state;
    }

private:
    static inline thread_local HookState* g_current_state = nullptr;
};
#endif

// Calling convention friendly implementation of std::is_function_v
template <class T>
constexpr bool is_function_v = !std::is_const_v<T> && !std::is_reference_v<T>;

#endif

}

#if defined(RCMP_HAS_HOOK_PROLOG_POLICY)

template <class Tag, class Signature, class F>
void hook_function(rcmp::address_t function_address, F&& hook) {
    using wrapped_policy_t = detail::WithGlobalState<
        detail::HookPrologStatelessPolicy,
        Tag
    >;
    rcmp::generic_hook_function<
        wrapped_policy_t::template Policy,
        Signature
    >(function_address, std::forward<F>(hook));
}

template <auto FunctionAddress, class Signature, class F>
void hook_function(F&& hook) {
    static_assert(std::is_constructible_v<rcmp::address_t, decltype(FunctionAddress)>);

    using Tag = std::integral_constant<decltype(FunctionAddress), FunctionAddress>;
    rcmp::hook_function<Tag, Signature>(FunctionAddress, std::forward<F>(hook));
}

template <auto Function, class F>
void hook_function(F&& hook) {
    using Signature = decltype(Function);

    static_assert(std::is_pointer_v<Signature>,                            "Function is not a _pointer_ to function. Did you forget to specify signature? (rcmp::hook_function<.., Signature>(..) overload)");
    static_assert(detail::is_function_v<std::remove_pointer_t<Signature>>, "Function is not a pointer to _function_. Did you forget to specify signature? (rcmp::hook_function<.., Signature>(..) overload)");

    using Tag = std::integral_constant<Signature, Function>;
    rcmp::hook_function<Tag, Signature>(rcmp::bit_cast<const void*>(Function), std::forward<F>(hook));
}

template <class Signature, class F>
void hook_function(rcmp::address_t function_address, F&& hook) {
    rcmp::hook_function<class Tag, Signature>(function_address, std::forward<F>(hook));
}

#if RCMP_GET_ARCH() == RCMP_ARCH_X86
template <class Signature, class F>
void hook_function_stateless(rcmp::address_t function_address, F&& hook) {
    rcmp::generic_hook_function<
        detail::HookPrologTlsStatePolicy,
        Signature
    >(function_address, std::forward<F>(hook));
}
#endif

#endif

}
