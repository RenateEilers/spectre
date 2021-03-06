cmake_minimum_required (VERSION 3.6.3)

################################################################
project (Spectre)
################################################################

# require the compiler to use C++14
# can't use C++17 since Bison requires the register storage, which is not compatible with C++17
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# add directoy, where we store all custom files for finding libraries which are not build using cmake (i.e. currently nothing), to the search path of cmake
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# add top level directory to the search path of compiler
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Set the output folder where the binary will be created (i.e. build/dir)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(spectre src/main.cpp)


add_subdirectory(src/analysis)
add_subdirectory(src/declarations)
add_subdirectory(src/logic)
add_subdirectory(src/parser)
add_subdirectory(src/program)
add_subdirectory(src/util)

target_link_libraries(spectre analysis declarations logic parser program util)
