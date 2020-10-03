add_executable(rcmp-tests test_hooks.cpp test_conv_meta.cpp main.cpp)

target_link_libraries(rcmp-tests PRIVATE rcmp)

if(MSVC)
    target_compile_options(rcmp-tests PRIVATE /W4 /WX)
else()
    target_compile_options(rcmp-tests PRIVATE -Wall -Wextra -pedantic -Wno-attributes -Werror)
endif()
