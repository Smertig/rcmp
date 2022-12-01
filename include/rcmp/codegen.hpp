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
