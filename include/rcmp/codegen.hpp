#pragma once

#include "detail/hook_installer.hpp"
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

} // namespace rcmp

#include "detail/hook_policy/prolog_policy.hpp"
#include "detail/hook_policy/indirect_policy.hpp"
