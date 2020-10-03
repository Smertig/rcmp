#pragma once

#include "low_level.hpp"
#include "detail/address.hpp"

#include <type_traits>
#include <memory>

#include <cstdint>
#include <cstddef>
#include <cstring>

namespace rcmp {

void unprotect_memory(rcmp::address_t where, std::size_t count);

template <class Range>
void set_opcode(rcmp::address_t where, Range&& bytes) {
    static_assert(sizeof(bytes.data()[0]) == 1);

    unprotect_memory(where, std::size(bytes));

    std::memcpy(where.as_ptr(), std::data(bytes), std::size(bytes));
}

std::size_t opcode_length(rcmp::address_t address);

std::unique_ptr<std::byte[]> allocate_code(std::size_t count);

} // namespace rcmp
