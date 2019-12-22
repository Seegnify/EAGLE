#!/bin/bash

#
# Copyright (c) 2019 Greg Padiasek
# Distributed under the terms of the MIT License.
# See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT
#

get_mnist()
{
  if [ ! -f "$1" ]; then
    wget "http://yann.lecun.com/exdb/mnist/$1.gz"
    gunzip "$1.gz"
  fi
}

# If not already downloaded
get_mnist train-images-idx3-ubyte
get_mnist train-labels-idx1-ubyte
get_mnist t10k-images-idx3-ubyte
get_mnist t10k-labels-idx1-ubyte

