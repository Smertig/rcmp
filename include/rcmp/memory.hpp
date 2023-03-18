#pragma once

#include "detail/address.hpp"

#include <memory>
#include <iterator>

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

std::unique_ptr<std::byte[]> allocate_code(std::size_t count);

} // namespace rcmp
