#!/bin/bash

#
# Copyright (c) 2019 Greg Padiasek
# Distributed under the terms of the the 3-Clause BSD License.
# See the accompanying file LICENSE or the copy at 
# https://opensource.org/licenses/BSD-3-Clause
#

# If not already downloaded
if [ ! -f ./cifar-10-batches-bin/data_batch_1.bin ]; then
    # If the archive does not exist, download it
    if [ ! -f ./cifar-10-binary.tar.gz ]; then
        wget https://www.cs.toronto.edu/~kriz/cifar-10-binary.tar.gz
    fi

    # Extract all the files
    tar xf cifar-10-binary.tar.gz
fi
