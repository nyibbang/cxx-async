cmake_minimum_required(VERSION 3.15)
project(cxx-async CXX)

# add_compile_options(-fcoroutines)

add_subdirectory(boost EXCLUDE_FROM_ALL)
find_package(Threads)

add_library(cxx_standard INTERFACE)
target_compile_features(cxx_standard INTERFACE cxx_std_20)

# ASIO
add_executable(
    cxx_async_asio
    src/main_asio.cpp
)
target_link_libraries(
    cxx_async_asio
    PUBLIC
        cxx_standard
        Boost::outcome
        Boost::asio
        Threads::Threads
)

# Cobalt
add_executable(
    cxx_async_cobalt
    src/main_cobalt.cpp
)
target_link_libraries(
    cxx_async_cobalt
    PUBLIC
        cxx_standard
        Boost::outcome
        Boost::cobalt
        Threads::Threads
)
