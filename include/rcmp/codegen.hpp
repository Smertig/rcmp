#pragma once

#include "detail/hook_installer/global_state.hpp"
#include "detail/address.hpp"
#include "detail/hook_state.hpp"

#include <type_traits>
#include <utility>

namespace rcmp {

template <template <class> class Policy, class Signature, class Hook>
void generic_hook_function(rcmp::address_t original_address, Hook&& hook) {
    detail::hook_installer<
        to_generic_signature<Signature>,
        std::decay_t<Hook>
    >::template install_hook<Policy>(original_address, std::forward<Hook>(hook));
}

// TODO: remove this overload
template <template <class> class Policy, auto Address, class Signature, class Hook>
void generic_hook_function(Hook&& hook) {
    static_assert(std::is_constructible_v<rcmp::address_t, decltype(Address)>);

    return rcmp::generic_hook_function<
        Policy,
        Signature
    >(Address, std::forward<Hook>(hook));
}

} // namespace rcmp

#include "detail/hook_policy/prolog_policy.hpp"
#include "detail/hook_policy/indirect_policy.hpp"
