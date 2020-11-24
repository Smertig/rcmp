#pragma once

#include <rcmp/low_level.hpp>

#include <cstdint>

namespace rcmp {

class address_t {
    static_assert(sizeof(std::uintptr_t) == sizeof(void*));

    std::uintptr_t m_value;

public:
    constexpr /* implicit */ address_t(std::nullptr_t = nullptr) noexcept : m_value(0) {
        // empty
    }

    constexpr /* implicit */ address_t(std::uintptr_t value) noexcept : m_value(value) {
        // empty
    }

    /* implicit */ address_t(const void* value) noexcept : address_t(bit_cast<std::uintptr_t>(value)) {
        // empty
    }

    template <class T>
    T as() const noexcept {
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(sizeof(T) == sizeof(void*));

        return bit_cast<T>(m_value);
    }

    template <class T = void>
    T* as_ptr() const noexcept {
        return as<T*>();
    }

    constexpr std::uintptr_t as_number() const noexcept {
        return m_value;
    }

#define RCMP_ADDRESS_DECLARE_OPERATOR(OP) friend constexpr bool operator OP(address_t lhs, address_t rhs) noexcept { return lhs.m_value OP rhs.m_value; }

    RCMP_ADDRESS_DECLARE_OPERATOR(<);
    RCMP_ADDRESS_DECLARE_OPERATOR(>);
    RCMP_ADDRESS_DECLARE_OPERATOR(<=);
    RCMP_ADDRESS_DECLARE_OPERATOR(>=);
    RCMP_ADDRESS_DECLARE_OPERATOR(==);
    RCMP_ADDRESS_DECLARE_OPERATOR(!=);

#undef RCMP_ADDRESS_DECLARE_OPERATOR

    friend constexpr address_t operator+(address_t lhs, std::ptrdiff_t rhs) noexcept { return { lhs.m_value + rhs }; }
    friend constexpr address_t operator+(std::ptrdiff_t lhs, address_t rhs) noexcept { return { lhs + rhs.m_value }; }

    friend constexpr address_t operator-(address_t lhs, std::ptrdiff_t rhs) noexcept { return { lhs.m_value - rhs }; }
    friend constexpr std::ptrdiff_t operator-(address_t lhs, address_t rhs) noexcept { return lhs.m_value - rhs.m_value; }

    constexpr address_t& operator+=(std::ptrdiff_t delta) noexcept { return (*this) = (*this) + delta; }
    constexpr address_t& operator-=(std::ptrdiff_t delta) noexcept { return (*this) = (*this) - delta; }
};

} // namespace rcmp
