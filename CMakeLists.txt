#
# Copyright (c) 2019 Greg Padiasek
# Distributed under the terms of the MIT License.
# See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT
#

#
# EAGLE
#

# CMakeLists files in this project can
# refer to the root source directory and root bnary directory of the project as
# respectively ${EAGLE_SOURCE_DIR} and ${EAGLE_BINARY_DIR}.
cmake_minimum_required (VERSION 2.8.11)
project (EAGLE)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DRELEASE -O3 -g")

# Show compiler stdout
set(CMAKE_VERBOSE_MAKEFILE ON)

# Enable C++11
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# Skip rpath settings
set(CMAKE_SKIP_RPATH TRUE)

# Define external include dirs
include_directories(/usr/include/eigen3)

# Add executable sources
add_executable (eagle
main.cc
base64.c
master.cc
worker.cc
storage.cc
eagle.pb.cc)

# Find dependency libs
list(APPEND DL_LIBS pthread protobuf dl)
list(APPEND DL_LIBS PocoFoundation PocoNet)

target_link_libraries(eagle ${DL_LIBS})
