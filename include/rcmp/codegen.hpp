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
struct hook_state_t {
    using original_sig_t = rcmp::from_generic_signature<GenericSignature>;
    using hook_t         = Hook;

    std::optional<hook_t> hook     = std::nullopt;
    original_sig_t        original = nullptr;
};

template <class GenericSignature, class Hook, class... Tags>
class hook_impl;

template <class Ret, class... Args, cconv Convention, class Hook, class... Tags>
class hook_impl<generic_signature_t<Ret(Args...), Convention>, Hook, Tags...> {
    using generic_sig_t  = generic_signature_t<Ret(Args...), Convention>;
    using original_sig_t = rcmp::from_generic_signature<generic_sig_t>;
    using hook_t         = Hook;

    static_assert(std::is_invocable_r_v<Ret, hook_t, original_sig_t, Args...>);

    inline static hook_state_t<generic_sig_t, hook_t> m_state;

    static Ret call_hook_impl(Args... args) {
        assert(m_state.hook != std::nullopt);
        assert(m_state.original != nullptr);
        return (*m_state.hook)(m_state.original, std::forward<Args>(args)...);
    }

    static inline constexpr original_sig_t call_hook = with_signature<call_hook_impl, generic_sig_t>;

public:
    template <class Policy>
    static void do_hook(rcmp::address_t address, hook_t hook) {
        if (m_state.original != nullptr) {
            throw rcmp::error("double hook of %" PRIXPTR, address.as_number());
        }

        m_state.hook.emplace(std::move(hook));
        m_state.original = Policy::make_raw_hook(address, rcmp::bit_cast<void*>(call_hook)).template as<original_sig_t>();
    }
};

} // namespace detail

template <class Policy, class Signature, class... Tags, class Hook>
void generic_hook_function(rcmp::address_t original_address, Hook&& hook) {
    detail::hook_impl<
        to_generic_signature<Signature>,
        std::decay_t<Hook>,
        Hook, Tags...
    >::template do_hook<Policy>(original_address, std::forward<Hook>(hook));
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
