cmake_minimum_required(VERSION 2.8)

project(consistent_weighted_sampling)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

# C++17 compiler check
if (CMAKE_CXX_COMPILER MATCHES ".*clang.*" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_COMPILER_IS_CLANGXX 1)
endif ()
if (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    set(CMAKE_COMPILER_IS_INTEL 1)
endif ()
if ((CMAKE_COMPILER_IS_GNUCXX AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 7.0) OR (CMAKE_COMPILER_IS_CLANGXX AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 4.0))
    message(FATAL_ERROR "Your C++ compiler does not support C++17. Please install g++ 7.0 (or greater) or clang 4.0 (or greater)")
else ()
    message(STATUS "Compiler is recent enough to support C++17.")
endif ()

set(GCC_WARNINGS "-Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z -pthread ${GCC_WARNINGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -march=native -O3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer -O0 -ggdb -DDEBUG")

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xpreprocessor -fopenmp")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
endif()

message(STATUS "BUILD_TYPE is ${CMAKE_BUILD_TYPE}")
message(STATUS "CXX_FLAGS are ${CMAKE_CXX_FLAGS}")
message(STATUS "CXX_FLAGS_DEBUG are ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CXX_FLAGS_RELEASE are ${CMAKE_CXX_FLAGS_RELEASE}")

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    include_directories(/usr/local/include /usr/local/opt/openssl/include)
    link_directories(/usr/local/lib /usr/local/opt/openssl/lib)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB SOURCES src/*.cpp)
foreach(SOURCE ${SOURCES})
    MESSAGE("TARGET:" ${SOURCE})
    get_filename_component(PREFIX ${SOURCE} NAME_WE)
    add_executable(${PREFIX} ${SOURCE})
    if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        target_link_libraries(${PREFIX} omp)
    endif()
endforeach(SOURCE)

message(STATUS "COPY ${CMAKE_SOURCE_DIR}/scripts to ${CMAKE_CURRENT_BINARY_DIR}")
file(COPY ${CMAKE_SOURCE_DIR}/scripts DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

