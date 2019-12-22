/**
 * Copyright (c) 2019 Greg Padiasek
 * Distributed under the terms of the MIT License.
 * See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT
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

