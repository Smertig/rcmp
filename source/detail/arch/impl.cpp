#include <rcmp/detail/config.hpp>

#if RCMP_GET_ARCH() == RCMP_ARCH_X86
    static_assert(sizeof(void*) == 4);
    #include "x86/impl.cpp"
#elif RCMP_GET_ARCH() == RCMP_ARCH_X86_64
    static_assert(sizeof(void*) == 8);
    #include "x86-64/impl.cpp"
#elif RCMP_GET_ARCH() == RCMP_ARCH_ARM64
    static_assert(sizeof(void*) == 8);
    #include "arm64/impl.cpp"
#else
	#error Unknown architecture
#endif
