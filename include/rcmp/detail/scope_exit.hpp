#pragma once

#include <type_traits>
#include <utility>

namespace rcmp::detail {

template <class F>
class scope_exit {
    F m_callback;

public:
    /* implicit */ scope_exit(F callback) : m_callback(std::move(callback)) {}
    ~scope_exit() noexcept(std::is_nothrow_invocable_v<F>) { m_callback(); }

    scope_exit(const scope_exit&) = delete;
    scope_exit& operator=(const scope_exit&) = delete;
    scope_exit(scope_exit&&) = delete;
    scope_exit& operator=(scope_exit&&) = delete;
};

}