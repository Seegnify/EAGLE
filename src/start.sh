#
# Copyright (c) 2019 Greg Padiasek
# Distributed under the terms of the the 3-Clause BSD License.
# See the accompanying file LICENSE or the copy at 
# https://opensource.org/licenses/BSD-3-Clause
#

IMPL=mnist
PORT=2020
./build/eagle master $IMPL.eagle $PORT >> master.log 2>&1 &
./build/eagle worker 127.0.0.1 $PORT ./examples/build/lib$IMPL.so >> worker.log 2>&1 &
