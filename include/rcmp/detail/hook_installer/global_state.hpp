#pragma once

#include <rcmp/detail/calling_convention.hpp>
#include <rcmp/detail/address.hpp>
#include <rcmp/detail/exception.hpp>
#include <rcmp/detail/hook_state.hpp>
#include <rcmp/low_level.hpp>

#include <utility>

namespace rcmp::detail {

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

}