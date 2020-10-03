#include <rcmp/memory.hpp>

std::unique_ptr<std::byte[]> rcmp::allocate_code(std::size_t count) {
    auto result = std::make_unique<std::byte[]>(count);

    unprotect_memory(result.get(), count);

    return result;
}
