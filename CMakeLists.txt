cmake_minimum_required(VERSION 3.21)
project(csci436-p1 C CXX)

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
target_include_directories(netcopy PRIVATE include)
target_link_libraries(netcopy PRIVATE fmt::fmt)
add_target_warnings(netcopy)
