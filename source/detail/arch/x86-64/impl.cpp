#include "../x86-64-common/impl.cxx"

#include <limits>

std::size_t rcmp::opcode_length(rcmp::address_t address) {
    return nmd_x86_ldisasm(address.as_ptr(), (std::numeric_limits<std::size_t>::max)(), NMD_X86_MODE_64);
}
