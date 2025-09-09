macro(add_target_warnings target)
    if(NOT MSVC)
        set(WARNING_FLAGS
            -Wall
            -pedantic
            -Wextra
            -Wno-unused-parameter
            -Wformat=2
            ${WPILIB_TARGET_WARNINGS}
        )
        if(NOT NO_WERROR)
            set(WARNING_FLAGS ${WARNING_FLAGS} -Werror)
        endif()

        target_compile_options(${target} PRIVATE ${WARNING_FLAGS})
    endif()

    # Compress debug info with GCC
    if(
        (${CMAKE_BUILD_TYPE} STREQUAL "Debug" OR ${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo")
        AND ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU"
    )
        target_compile_options(${target} PRIVATE -gz=zlib)
    endif()
endmacro()
