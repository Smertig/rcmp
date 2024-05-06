project(rcmp-tests)

add_executable(rcmp-tests
        main.cpp
        test_hooks.cpp
        test_conv_meta.cpp
        # Validate that every single public header is able to compile without additional headers
        validate_headers/rcmp.cpp
        validate_headers/codegen.cpp
        validate_headers/low_level.cpp
        validate_headers/memory.cpp
        validate_headers/version.cpp)

target_link_libraries(rcmp-tests PRIVATE rcmp)
set_target_properties(rcmp-tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if(MSVC)
    target_link_options(rcmp-tests PRIVATE
        # Disable thunk functions, otherwise they have too small body (5 bytes - only single jump) to hook.
        /INCREMENTAL:NO
        # Don't merge identical functions into a single one
        /OPT:NOICF
    )
    target_compile_options(rcmp-tests PRIVATE /W4)
else()
    target_compile_options(rcmp-tests PRIVATE -Wall -Wextra -pedantic -Wno-attributes)
endif()
