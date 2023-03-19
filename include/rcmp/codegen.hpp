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

template <class GenericSignature, class Hook>
struct hook_state_t;

template <class Ret, class... Args, cconv Convention, class Hook>
struct hook_state_t<generic_signature_t<Ret(Args...), Convention>, Hook> {
    using generic_sig_t  = generic_signature_t<Ret(Args...), Convention>;
    using original_sig_t = rcmp::from_generic_signature<generic_sig_t>;
    using hook_t         = Hook;

    static_assert(std::is_invocable_r_v<Ret, hook_t, original_sig_t, Args...>);

    std::optional<hook_t> hook     = std::nullopt;
    original_sig_t        original = nullptr;

    Ret call_hook(Args... args) {
        assert(this->hook != std::nullopt);
        assert(this->original != nullptr);
        return (*this->hook)(this->original, std::forward<Args>(args)...);
    }
};

template <class GenericSignature, class Hook, class... Tags>
class global_state_hook_installer;

template <class Ret, class... Args, cconv Convention, class Hook, class... Tags>
class global_state_hook_installer<generic_signature_t<Ret(Args...), Convention>, Hook, Tags...> {
    using generic_sig_t  = generic_signature_t<Ret(Args...), Convention>;
    using original_sig_t = rcmp::from_generic_signature<generic_sig_t>;
    using hook_t         = Hook;

    inline static hook_state_t<generic_sig_t, hook_t> g_state;

    static Ret call_hook(Args... args) {
        return g_state.call_hook(std::forward<Args>(args)...);
    }

    inline static constexpr original_sig_t g_hook_with_fixed_cconv = with_signature<call_hook, generic_sig_t>;

public:
    template <class Policy>
    static void install_hook(rcmp::address_t address, hook_t hook) {
        if (g_state.original != nullptr) {
            throw rcmp::error("double hook of %" PRIXPTR, address.as_number());
        }

        g_state.hook.emplace(std::move(hook));
        g_state.original = Policy::install_raw_hook(address, rcmp::bit_cast<void*>(g_hook_with_fixed_cconv)).template as<original_sig_t>();
    }
};

} // namespace detail

template <class Policy, class Signature, class... Tags, class Hook>
void generic_hook_function(rcmp::address_t original_address, Hook&& hook) {
    detail::global_state_hook_installer<
        to_generic_signature<Signature>,
        std::decay_t<Hook>,
        Hook, Tags...
    >::template install_hook<Policy>(original_address, std::forward<Hook>(hook));
}

template <class Policy, auto Address, class Signature, class... Tags, class Hook>
void generic_hook_function(Hook&& hook) {
    static_assert(std::is_constructible_v<rcmp::address_t, decltype(Address)>);

    return rcmp::generic_hook_function<
        Policy,
        Signature,
        std::integral_constant<decltype(Address), Address>, Tags...
    >(Address, std::forward<Hook>(hook));
}

} // namespace rcmp

#include "hook_policy/hook_prolog_policy.hpp"
#include "hook_policy/hook_indirect_policy.hpp"
