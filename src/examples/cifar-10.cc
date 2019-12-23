/**
 * Copyright (c) 2019 Greg Padiasek
 * Distributed under the terms of the the 3-Clause BSD License.
 * See the accompanying file LICENSE or the copy at 
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include "cifar-10.hh"

///////////////////////////////////
extern "C" { // export C signatures
///////////////////////////////////

NeuroEvolution* create()
{
  return new EvolutionImpl_cifar10();
}

void destroy(NeuroEvolution* ptr)
{
  delete (EvolutionImpl_cifar10*)ptr;
}

///////////////////////////////////
} // export C signatures
///////////////////////////////////

