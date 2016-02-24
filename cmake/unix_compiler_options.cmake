set(PDAL_COMMON_CXX_FLAGS "-Werror -Wextra -Wall -Wno-unused-parameter -Wno-unused-variable -Wpointer-arith -Wcast-align -Wcast-qual -Wredundant-decls -Wno-long-long -Wno-unknown-pragmas -isystem /usr/local/include"
)

if (${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
    if (${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 4.7)
       set(CXX_STANDARD "-std=c++0x")
    else()
       set(CXX_STANDARD "-std=c++11")
    endif()
    if (${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER 4.6)
        set(PDAL_NO_AS_NEEDED_START "-Wl,--no-as-needed")
        set(PDAL_NO_AS_NEEDED_END "-Wl,--as-needed")
    endif()
    set(PDAL_COMPILER_GCC 1)
elseif (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    set(CXX_STANDARD "-std=c++11")
    set(PDAL_COMPILER_CLANG 1)
else()
    message(FATAL_ERROR "Unsupported C++ compiler")
endif()

set (CMAKE_CXX_FLAGS "${PDAL_COMMON_CXX_FLAGS} ${CXX_STANDARD}")
