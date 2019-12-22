/**
 * Copyright (c) 2019 Greg Padiasek
 * Distributed under the terms of the MIT License.
 * See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT
 */

#include <random>

#ifndef _RANDOM_NUMBER_GENERATOR_H_
#define _RANDOM_NUMBER_GENERATOR_H_

class RNG
{
public:
  RNG()
  {
    seed();
  }

  void seed()
  {
    generator.seed(device());
  }

  int uniform_int(int top)
  {
    std::uniform_int_distribution<int> d(0, top);
    return d(generator);
  }

  int uniform_int(int min, int max)
  {
    std::uniform_int_distribution<int> d(min, max);
    return d(generator);
  }

  float uniform_dec(float top)
  {
    std::uniform_real_distribution<float> d(0.0, top);
    return d(generator);
  }

  float uniform_dec(float min, float max)
  {
    std::uniform_real_distribution<float> d(min, max);
    return d(generator);
  }
  
  float normal_dec(float mean, float stddev)
  {
     std::normal_distribution<float> d(mean, stddev);
    return d(generator);
  }

  template<class Iterator>
  int discrete_choice(Iterator first, Iterator last)
  {
    std::discrete_distribution<> d(first, last);
    return d(generator);
  }

  template<class Iterator>
  void shuffle(Iterator first, Iterator last)
  {
    std::shuffle(first, last, generator);
  }

private:
  std::random_device device;
  std::mt19937 generator;
};

#endif /*_RANDOM_NUMBER_GENERATOR_H_*/
