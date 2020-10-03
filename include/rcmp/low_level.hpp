#pragma once

#include <type_traits>
#include <cstdint>
#include <cstring>

#include "detail/calling_convention.hpp"

namespace rcmp {

namespace detail {
    template <class PMF, cconv Convention>
    struct flatten_pmf_impl;

    template <class Cls, class Ret, class... Args, cconv Convention>
    struct flatten_pmf_impl<Ret(Cls::*)(Args...), Convention> {
        using type = typename from_generic_signature_impl<generic_signature_t<Ret(Cls*, Args...), Convention>>::type;
    };

    template <class Cls, class Ret, class... Args, cconv Convention>
    struct flatten_pmf_impl<Ret(Cls::*)(Args...) const, Convention> {
        using type = typename from_generic_signature_impl<generic_signature_t<Ret(const Cls*, Args...), Convention>>::type;
    };
} // namespace detail

template <class PMF, cconv Convention>
using flatten_pmf_t = typename detail::flatten_pmf_impl<PMF, Convention>::type;

template <class U, class T>
U bit_cast(T t) noexcept {
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(std::is_trivially_copyable_v<U>);
    static_assert(sizeof(T) == sizeof(U), "size mismatch");

    U u;
    std::memcpy(&u, &t, sizeof(t));
    return u;
}

} // namespace rcmp
