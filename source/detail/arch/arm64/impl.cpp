#include <rcmp/memory.hpp>

/*
 * No implementation, user should provide its own
 *
 * // returns relocated original address
 * rcmp::address_t rcmp::make_raw_hook(rcmp::address_t original_function, rcmp::address_t wrapper_function);
*/

std::size_t rcmp::opcode_length(rcmp::address_t address) {
    static_cast<void>(address);
    return 4;
}
