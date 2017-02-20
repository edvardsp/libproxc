# Distributed under MIT License (See accompanying file LICENSE)
# This CMakeLists is heavily inspired by boost Hana's CMakeLists (https://github.com/boostorg/hana)

cmake_minimum_required(VERSION 3.1 FATAL_ERROR)


################################################################################
# Setup project
################################################################################

set(proxc_VERSION_STRING "2.0.0")
project(proxc VERSION ${proxc_VERSION_STRING} LANGUAGES CXX)


################################################################################
# Setup library target
################################################################################

add_library(proxc INTERFACE)
target_include_directories(proxc INTERFACE include)


################################################################################
# Setup CMake options
################################################################################

option(PROXC_ENABLE_WERROR     "Early fail and stop when encountering a warning." OFF)
option(PROXC_ENABLE_MEMCHECK   "Run tests and examples under Valgrind if Valgrind is found." OFF)
option(PROXC_ENABLE_DEBUG_MODE "Enable debug mode." OFF)
option(PROXC_ENABLE_EXCEPTIONS "Build with exceptions enabled." ON)

################################################################################
# Setup compiler flags
################################################################################

include(CheckCXXCompilerFlag)
macro(proxc_append_flag testname flag)
    check_cxx_compiler_flag(${flag} ${testname})
    if (${testname})
        add_compile_options(${flag})
    endif()
endmacro()

# Compiler flags set by CMake options
if (PROXC_ENABLE_WERROR)
    proxc_append_flag(PROXC_HAS_WERROR -Werror)
    proxc_append_flag(PROXC_HAS_WX -WX)
endif()

if (PROXC_ENABLE_DEBUG_MODE)
    add_definitions(-DPROXC_CONFIG_ENABLE_DEBUG_MODE)
endif()

if (NOT PROXC_ENABLE_EXCEPTIONS)
    proxc_append_flag(PROXC_HAS_FNO_EXCEPTIONS -fno-exceptions)
endif()

# Other compiler flags
proxc_append_flag(PROXC_HAS_FDIAGNOSTICS_COLOR         -fdiagnostics-color)
proxc_append_flag(PROXC_HAS_FTEMPLATE_BACKTRACE_LIMIT  -ftemplate-backtrace-limit=0)
proxc_append_flag(PROXC_HAS_PEDANTIC                   -pedantic)
proxc_append_flag(PROXC_HAS_STDCXX1Y                   -std=c++1y)
proxc_append_flag(PROXC_HAS_QUNUSED_ARGUMENTS          -Qunused-arguments)
proxc_append_flag(PROXC_HAS_W                          -W)
proxc_append_flag(PROXC_HAS_WALL                       -Wall)
proxc_append_flag(PROXC_HAS_WEXTRA                     -Wextra)
proxc_append_flag(PROXC_HAS_WNO_UNUSED_LOCAL_TYPEDEFS  -Wno-unused-local-typedefs)
proxc_append_flag(PROXC_HAS_WWRITE_STRINGS             -Wwrite-strings)


################################################################################
# Setup include paths
################################################################################

include_directories(${PROJECT_SOURCE_DIR}/include)


################################################################################
# Setup custom functions to ease creation of targets
################################################################################

#   proxc_add_test(<name> <command> [<arg>...])
#
# Creates a test called `name`, which runs the given `command` with the given 
# `arg`uments. However, if `PROXC_ENABLE_MEMCHECK` is set to `ON`, the
# test will run the provided command under the memory checker.
if (PROXC_ENABLE_MEMCHECK)
    find_package(Valgrind REQUIRED)
    function(proxc_add_test name)
        add_test(${name} ${Valgrind_EXECUTABLE} --leak-check=full --error-exitcode=1 ${ARGN})
    endfunction()
else()
    function(proxc_add_test name)
        add_test(${name} ${ARGN})
    endfunction()
endif()


################################################################################
# Setup the `check` target to build and then run all the tests and examples.
################################################################################

add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Build and then run all the tests and examples.")


################################################################################
# Setup subdirectories
################################################################################

enable_testing()
add_subdirectory(examples)
add_subdirectory(tests)


################################################################################
# Setup the `install` target.
# This copies the whole content of include/ to ${CMAKE_INSTALL_PREFIX}
################################################################################

install(TARGETS proxc
    ARCHIVE DESTINATION proxc
    LIBRARY DESTINATION proxc
    RUNTIME DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)

