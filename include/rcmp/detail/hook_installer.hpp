#pragma once

#include <rcmp/detail/calling_convention.hpp>
#include <rcmp/detail/address.hpp>
#include <rcmp/detail/hook_state.hpp>
#include <rcmp/detail/scope_exit.hpp>
#include <rcmp/low_level.hpp>

#include <utility>

namespace rcmp::detail {

template <class GenericSignature, class Hook>
class hook_installer;

template <class Ret, class... Args, cconv Convention, class Hook>
class hook_installer<generic_signature_t<Ret(Args...), Convention>, Hook> {
    using generic_sig_t  = generic_signature_t<Ret(Args...), Convention>;
    using original_sig_t = rcmp::from_generic_signature<generic_sig_t>;
    using hook_t         = Hook;
    using state_t        = hook_state_t<generic_sig_t, hook_t>;

    template <class Policy>
    static Ret call_hook(Args... args) {
        const auto state = Policy::get_state();
        assert(state != nullptr);
        scope_exit _ = []{ Policy::set_state(nullptr); };

        return state->call_hook(std::forward<Args>(args)...);
    }

public:
    template <template <class HookState> class Policy>
    static void install_hook(rcmp::address_t address, hook_t hook) {
        using policy_t = Policy<state_t>;
        constexpr original_sig_t hook_with_fixed_cconv = with_signature<call_hook<policy_t>, generic_sig_t>;

        const auto state = policy_t::allocate_state(address);
        state->hook.emplace(std::move(hook));
        state->original = policy_t::install_raw_hook(state, address, rcmp::bit_cast<void*>(hook_with_fixed_cconv)).template as<original_sig_t>();
    }
};

}