# Target name
set(target conquer-the-spire)

# Define
set(root_dir ${CMAKE_CURRENT_SOURCE_DIR}/../..)

# Includes
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Libraries
)

# Sources
file(GLOB header_dir
    ${root_dir}/Includes)

file(GLOB_RECURSE headers
    ${header_dir}/*.hpp)

file(GLOB_RECURSE sources
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# Custom-build event
set(baba_is_auto_header_gen_py ${root_dir}/Scripts/header_gen.py)
add_custom_command(
    COMMAND python ${baba_is_auto_header_gen_py}
    DEPENDS ${headers}
    OUTPUT ${root_dir}/Includes/baba-is-auto.hpp
)
add_custom_target(baba_is_auto_header_gen_py ALL
    DEPENDS ${root_dir}/Includes/baba-is-auto.hpp) 

# Build library
add_library(${target}
    ${sources})
add_dependencies(${target} baba_is_auto_header_gen_py)

# A shared build of the same sources, so that a learner in another language
# can reach the C interface with nothing but a shared library.
add_library(${target}-shared SHARED
    ${sources})
add_dependencies(${target}-shared baba_is_auto_header_gen_py)

set_target_properties(${target}-shared
    PROPERTIES
    ${DEFAULT_PROJECT_OPTIONS}
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME ${target}
)

target_compile_options(${target}-shared
    PUBLIC
    ${DEFAULT_COMPILE_OPTIONS}
)

target_link_libraries(${target}-shared
    PUBLIC
    ${DEFAULT_LINKER_OPTIONS}
    ${DEFAULT_LIBRARIES}
)

# Fold the GCC runtime into the library itself. Python 3.8 onwards stopped
# looking along PATH for a DLL's own dependencies, so a library that still
# needed libstdc++-6.dll beside it would not load from ctypes at all.
# -static rather than the two -static-lib* flags on their own: the POSIX
# threads build of the toolchain also pulls in libwinpthread-1.dll, which
# would leave the same hole.
if(MINGW)
    target_link_options(${target}-shared
        PRIVATE
        -static
    )
endif()

install(TARGETS ${target}-shared DESTINATION lib)

# Project options
set_target_properties(${target}
    PROPERTIES
    ${DEFAULT_PROJECT_OPTIONS}
)

# Compile options
target_compile_options(${target}
    PRIVATE

    PUBLIC
    ${DEFAULT_COMPILE_OPTIONS}

    INTERFACE
)

target_link_libraries(${target}
    PRIVATE

    PUBLIC
    ${DEFAULT_LINKER_OPTIONS}
	${DEFAULT_LIBRARIES}

    INTERFACE
)

# Install
install(TARGETS ${target} DESTINATION lib)
install(DIRECTORY ${header_dir} DESTINATION include)