#pragma once

#include <stdexcept>
#include <cstdio>
#include <cinttypes>
#include <string>

namespace rcmp {

class error : public std::exception {
    std::string m_message;

public:
    template <class... Args>
    explicit error(const char* fmt, const Args& ...args) {
        if constexpr (sizeof...(Args) > 0) {
            const auto len = std::snprintf(nullptr, 0, fmt, args...);

            m_message.resize(len + 1);
            snprintf(m_message.data(), m_message.size(), fmt, args...);
            m_message.pop_back(); // drop '\0'
        }
        else {
            m_message = fmt;
        }
    }

    const char* what() const noexcept final {
        return m_message.c_str();
    }
};

} // namespace rcmp
