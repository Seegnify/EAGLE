/**
 * Copyright (c) 2019 Greg Padiasek
 * Distributed under the terms of the MIT License.
 * See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT
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

