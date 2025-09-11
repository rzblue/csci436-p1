cmake_minimum_required(VERSION 3.21)
project(csci436-p1 C CXX)

if(WIN32)
    message(FATAL_ERROR "Windows targets are not supported")
endif()

option(NO_WERROR "Disable warnings-as-errors (-Werror)" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(target_warnings.cmake)
include(FetchContent)

FetchContent_Declare(
    fmt
    URL https://github.com/fmtlib/fmt/releases/download/11.2.0/fmt-11.2.0.zip
    URL_HASH SHA256=203EB4E8AA0D746C62D8F903DF58E0419E3751591BB53FF971096EAA0EBD4EC3
    DOWNLOAD_EXTRACT_TIMESTAMP true
)
FetchContent_MakeAvailable(fmt)

file(GLOB_RECURSE netcopy_src "src/*.cpp" "src/*.c")

add_executable(netcopy ${netcopy_src})
add_custom_target(run
    COMMAND netcopy
    DEPENDS netcopy
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
)
target_compile_features(netcopy PRIVATE cxx_std_20)
target_include_directories(netcopy PRIVATE include)
target_link_libraries(netcopy PRIVATE fmt::fmt)
add_target_warnings(netcopy)
