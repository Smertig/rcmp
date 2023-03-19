#pragma once

#include "detail/hook_installer/global_state.hpp"
#include "detail/address.hpp"
#include "detail/hook_state.hpp"

#include <type_traits>
#include <utility>

namespace rcmp {

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
