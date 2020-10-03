#include <rcmp/memory.hpp>
#include <rcmp/detail/exception.hpp>

#include <Windows.h>

void rcmp::unprotect_memory(rcmp::address_t where, std::size_t count) {
    if (count == 0) {
        return;
    }

    DWORD protect;
    const auto result = VirtualProtect(where.as_ptr(), count, PAGE_EXECUTE_READWRITE, &protect);

    if (result == FALSE) {
        throw rcmp::error("VirtualProtect fails with error %lu", ::GetLastError());
    }
}
