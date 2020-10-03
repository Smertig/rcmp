#include <sys/mman.h>
#include <errno.h>
#include <limits.h>

#include <rcmp/memory.hpp>
#include <rcmp/detail/exception.hpp>

void rcmp::unprotect_memory(rcmp::address_t where, std::size_t count) {
    if (count == 0) {
        return;
    }

    constexpr const std::uintptr_t page_size = 0x1000;
    constexpr const std::uintptr_t page_mask = page_size - 1;

    rcmp::address_t page_begin = where.as_number() & ~page_mask;
    rcmp::address_t page_end = (where + count - 1).as_number() & ~page_mask;
    for (; page_begin <= page_end; page_begin += page_size) {
        if (::mprotect(page_begin.as_ptr(), page_size, PROT_EXEC | PROT_WRITE | PROT_READ)) {
            throw rcmp::error("mprotect(%" PRIXPTR ", %lu) fails with error: %s", page_begin, page_size, ::strerror(errno));
        }
    }
}
