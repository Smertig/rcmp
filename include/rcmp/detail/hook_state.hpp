#pragma once

#include <rcmp/detail/calling_convention.hpp>

#include <optional>
#include <utility>
#include <type_traits>
#include <cassert>

namespace rcmp::detail {

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

}
