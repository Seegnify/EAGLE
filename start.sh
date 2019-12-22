#
# Copyright (c) 2019 Greg Padiasek
# Distributed under the terms of the MIT License.
# See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT
#

IMPL=mnist
PORT=2020
./build/eagle master $IMPL.eagle $PORT >> master.log 2>&1 &
./build/eagle worker 127.0.0.1 $PORT ./examples/build/lib$IMPL.so >> worker.log 2>&1 &
