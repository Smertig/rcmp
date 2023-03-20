#pragma once

#include <rcmp/detail/exception.hpp>
#include <rcmp/memory.hpp>

namespace rcmp::detail {

template <class StatelessPolicy, class... Tags>
struct WithGlobalState {
    template <class HookState>
    class Policy {
        inline static HookState g_state;

    public:
        static rcmp::address_t install_raw_hook([[maybe_unused]] HookState* state, rcmp::address_t address, rcmp::address_t wrapper_function) {
            assert(state == &g_state);

            return StatelessPolicy::install_stateless_hook(address, wrapper_function);
        }

        static HookState* allocate_state(rcmp::address_t address) {
            static bool allocated = false;
            if (std::exchange(allocated, true)) {
                throw rcmp::error("double hook of %" PRIXPTR, address.as_number());
            }

            return &g_state;
        }

        static HookState* get_state() {
            return &g_state;
        }

        static void set_state(HookState* state) {
            assert(state == nullptr || state == &g_state);
        }
    };
};

}
