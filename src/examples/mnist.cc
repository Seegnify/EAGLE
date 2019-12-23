/**
 * Copyright (c) 2019 Greg Padiasek
 * Distributed under the terms of the the 3-Clause BSD License.
 * See the accompanying file LICENSE or the copy at 
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include "mnist.hh"

///////////////////////////////////
extern "C" { // export C signatures
///////////////////////////////////

NeuroEvolution* create()
{
  return new EvolutionImpl_mnist();
}

void destroy(NeuroEvolution* ptr)
{
  delete (EvolutionImpl_mnist*)ptr;
}

///////////////////////////////////
} // export C signatures
///////////////////////////////////

