cmake_minimum_required(VERSION 3.15)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()


project(
    "nginx-module-with-embedded-bpftime"
    VERSION 0.1.0
    LANGUAGES C CXX
)
option(ENABLE_LTO "Whether to enable LTO" NO)
if(${ENABLE_LTO})
    include(CheckIPOSupported)
    check_ipo_supported(RESULT result OUTPUT output)

    if(result)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(SEND_ERROR "IPO is not supported: ${output}.")
    endif()
endif()
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(bpftime)
set(CMAKE_CXX_VISIBILITY_PRESET default)
add_library(nginx-module-aux SHARED module_aux.cpp)
add_dependencies(nginx-module-aux runtime copy_headers)
target_include_directories(nginx-module-aux PRIVATE ${BPFTIME_RUNTIME_INCLUDE})
target_link_libraries(nginx-module-aux runtime)
include(ExternalProject)

# We need to download the source of nginx, since it requires them to build modules
ExternalProject_Add(
    nginx
    DOWNLOAD_COMMAND URL https://nginx.org/download/nginx-1.22.1.tar.gz
    CONFIGURE_COMMAND cd ${CMAKE_CURRENT_BINARY_DIR}/nginx-prefix/src/nginx && ./configure --add-dynamic-module=${CMAKE_BINARY_DIR}/../module --with-compat
    BUILD_COMMAND make -C ${CMAKE_CURRENT_BINARY_DIR}/nginx-prefix/src/nginx modules -j32
    INSTALL_COMMAND ""
    BUILD_ALWAYS TRUE
    DEPENDS nginx-module-aux
)

add_custom_command(
    TARGET nginx
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/module-output
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/nginx-prefix/src/nginx/objs/*.so ${CMAKE_SOURCE_DIR}/module-output/
    COMMENT "Copy nginx module to ${CMAKE_SOURCE_DIR}/module-output/"
)

add_dependencies(nginx runtime bpftime_vm)
