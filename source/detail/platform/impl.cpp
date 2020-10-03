#include <rcmp/detail/config.hpp>

#if RCMP_GET_PLATFORM() == RCMP_PLATFORM_LINUX
    #include "linux/impl.cpp"
#elif RCMP_GET_PLATFORM() == RCMP_PLATFORM_WIN
    #include "win/impl.cpp"
#else
    #error Unknown platform
#endif
