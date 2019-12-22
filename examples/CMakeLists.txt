#
# Copyright (c) 2019 Greg Padiasek
# Distributed under the terms of the MIT License.
# See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT
#

#
# EAGLE examples
#

# CMakeLists files in this project can
# refer to the root source directory and root bnary directory of the project as
# respectively ${EAGLE_SOURCE_DIR} and ${EAGLE_BINARY_DIR}.
cmake_minimum_required (VERSION 2.8.11)
project (IMPL)

# for _rdrandXX_step 
#SET(CMAKE_CXX_FLAGS "-mrdrnd")

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
include_directories(mnist/include)
include_directories(cifar-10/include)

# Add include directories from graph
include_directories(../)

# Add executable sources
add_library (mnist SHARED
mnist.cc
)

# Add executable sources
add_library (cifar-10 SHARED
cifar-10.cc
)

# Find dependency libs
list(APPEND DL_LIBS protobuf)

# Link targets
target_link_libraries(mnist ${DL_LIBS})
target_link_libraries(cifar-10 ${DL_LIBS})

