cmake_minimum_required(VERSION 3.16)
project(rcmp)

set(RCMP_VERSION_MAJOR 0)
set(RCMP_VERSION_MINOR 2)
set(RCMP_VERSION_PATCH 1)

set(RCMP_EXTERNAL_DIR ${PROJECT_SOURCE_DIR}/external)
set(RCMP_SOURCE_DIR ${PROJECT_SOURCE_DIR}/source)

add_library(rcmp STATIC
        ${RCMP_SOURCE_DIR}/codegen.cpp
        ${RCMP_SOURCE_DIR}/memory.cpp
        ${RCMP_SOURCE_DIR}/detail/arch/impl.cpp
        ${RCMP_SOURCE_DIR}/detail/platform/impl.cpp
        )

target_compile_features(rcmp PUBLIC cxx_std_17)

target_include_directories(rcmp PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${RCMP_EXTERNAL_DIR}/nmd/include
        )

target_compile_definitions(rcmp PUBLIC RCMP_VERSION_MAJOR=${RCMP_VERSION_MAJOR})
target_compile_definitions(rcmp PUBLIC RCMP_VERSION_MINOR=${RCMP_VERSION_MINOR})
target_compile_definitions(rcmp PUBLIC RCMP_VERSION_PATCH=${RCMP_VERSION_PATCH})

if(MSVC)
    target_compile_options(rcmp PRIVATE /W4 /WX)
else()
    target_compile_options(rcmp PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

if (${CMAKE_SOURCE_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
    add_subdirectory(test)
endif()
