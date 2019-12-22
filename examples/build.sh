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

# generate makefile
cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} --build ${ROOT_DIR}/build

# build
cmake --build ${ROOT_DIR}/build
if [ $? != 0 ]; then
  exit 2
fi

# download data
cd mnist
./download.sh
cd -

cd cifar-10
./download.sh
cd -

