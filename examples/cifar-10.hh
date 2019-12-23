/**
 * Copyright (c) 2019 Greg Padiasek
 * Distributed under the terms of the the 3-Clause BSD License.
 * See the accompanying file LICENSE or the copy at 
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include "eagle.hh"

#include <cifar/cifar10_reader.hpp>

class EvolutionImpl_cifar10 : public NeuroEvolution
{
public:
  EvolutionImpl_cifar10() : NeuroEvolution(3 * 32 * 32, 10, 4, 4, 50)
  {
    _epoch = 10;
    _objective = 1 - 1e-5;
    
    _data = cifar::read_dataset<std::vector, std::vector, uint8_t, uint8_t>
    ("examples/cifar-10/cifar-10-batches-bin");
    
    std::cout << " training_images=" << _data.training_images.size()
              << " training_labels=" << _data.training_labels.size()
              << " test_images=" << _data.test_images.size()
              << " test_labels=" << _data.test_labels.size()
              << std::endl;

    _training.resize(_data.training_images.size());
    for (int i=_data.training_images.size()-1; i>=0; i--) _training[i] = i;
  } 

protected:
  // mnist data
  cifar::CIFAR10_dataset<std::vector, std::vector<uint8_t>, uint8_t> _data;

  // data index
  std::vector<int> _training;

  // set input
  void set_input(Graph& g, std::vector<uint8_t>& image)
  {
    for (int rgb=0; rgb<3; rgb++)
    for (int r=0; r<32; r++)
    for (int c=0; c<32; c++)
    {
      g.set(rgb * 32 * 32 + r * 32 + c, image[rgb * 32 * 32 + r * 32 + c]);
    }
  }

  // get output
  int get_output(Graph& g)
  {
    int size = g._meta.output;
    std::vector<DTYPE> output(size);
    for (int i=0; i<size; i++) output[i] = g.get(i);
    return _rng.discrete_choice(output.begin(), output.end());
  }

  // train episode that updates graph weights and returns graph reward
  virtual DTYPE episode(Graph& g)
  {
    // set batch size
    int batch = 1000;

    // randomize training samples
    _rng.shuffle(_training.begin(), _training.end());

    // train on batch
    DTYPE R = 0;
    for (int i=0; i<batch; i++)
    {
      int ir = _training[i];
      auto& image = _data.training_images[ir];
      auto label = _data.training_labels[ir];

      g.reset();
      set_input(g, image);
      DTYPE y = get_output(g);
      DTYPE y_hat = label;
      DTYPE r = (y == y_hat) ? 1 : 0;
      g.reward(r);
      g.gradient();
      R += r;
    }
    g.update();

    // calculate fitness
    return R / batch;
  }

  DTYPE validate(Graph& g)
  {
    // set batch size
    int batch = _data.test_images.size();

    // validate on test
    DTYPE R = 0;
    for (int i=0; i<batch; i++)
    {
      auto& image = _data.test_images[i];
      auto label = _data.test_labels[i];

      g.reset();
      set_input(g, image);
      DTYPE y = get_output(g);
      DTYPE y_hat = label;
      DTYPE r = (y == y_hat) ? 1 : 0;
      R += r;
    }

    // calculate fitness
    return R / batch;
  }
};

