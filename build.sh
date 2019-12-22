#!/bin/sh

#
# Copyright (c) 2019 Greg Padiasek
# Distributed under the terms of the MIT License.
# See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT
#

ROOT_DIR=$(dirname "$0")

if [ -z "$BUILD_TYPE" ]; then
  BUILD_TYPE=Release
fi

# show build type
echo "Build: $BUILD_TYPE"

# create build folder
if [ ! -d "${ROOT_DIR}/build" ]; then
  mkdir "${ROOT_DIR}/build"
  cd "${ROOT_DIR}/build"
  cmake ..
  cd -
fi

# generate proto file
protoc -I="$ROOT_DIR" --cpp_out="$ROOT_DIR" "$ROOT_DIR/eagle.proto"

# generate makefile
cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} --build ${ROOT_DIR}/build

# build framework
cmake --build ${ROOT_DIR}/build

# build examples
if [ -d examples ]; then
  cd examples
  ./build.sh
fi
