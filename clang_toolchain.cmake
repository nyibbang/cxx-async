include(${CMAKE_CURRENT_LIST_DIR}/build/Debug/generators/conan_toolchain.cmake)

set(CMAKE_C_COMPILER clang-12)
set(CMAKE_CXX_COMPILER clang++-12)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
