cmake_minimum_required(VERSION 3.15)
project(cxx-async CXX)

add_library(cxx_standard INTERFACE)
target_compile_features(cxx_standard INTERFACE cxx_std_20)

find_package(Boost COMPONENTS asio)
find_package(Threads)

# ASIO
add_executable(
    cxx_async_asio
    src/main_asio.cpp
)
target_link_libraries(
    cxx_async_asio
    PUBLIC
        cxx_standard
        Boost::boost
        Threads::Threads
)
target_compile_options(
    cxx_async_asio
    PUBLIC
        $<$<CXX_COMPILER_ID:GNU>:-fcoroutines>
        $<$<CXX_COMPILER_ID:Clang>:-stdlib=libc++>
)
target_link_options(
    cxx_async_asio
    PUBLIC
        $<$<CXX_COMPILER_ID:Clang>:-stdlib=libc++ -lc++abi>
)
