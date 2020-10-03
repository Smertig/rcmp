#include <rcmp/memory.hpp>
#include <rcmp/codegen.hpp>

// returns relocated original address
rcmp::address_t rcmp::make_raw_hook(rcmp::address_t original_function, rcmp::address_t wrapper_function) {
    static_cast<void>(original_function);
    static_cast<void>(wrapper_function);
    throw rcmp::error("rcmp::make_raw_hook is not implemented yet");
}

std::size_t rcmp::opcode_length(rcmp::address_t address) {
    static_cast<void>(address);
    throw rcmp::error("rcmp::opcode_length is not implemented yet");
}
