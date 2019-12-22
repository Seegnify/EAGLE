/**
 * Copyright (c) 2019 Greg Padiasek
 * Distributed under the terms of the MIT License.
 * See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT
 */

#include "eagle.hh"

#include <mnist/mnist_reader.hpp>

class EvolutionImpl_mnist : public NeuroEvolution
{
public:
  EvolutionImpl_mnist() : NeuroEvolution(28 * 28, 10, 8, 2, 50)
  {
    _epoch = 10;
    _objective = 1 - 1e-5;
    
    _data = mnist::read_dataset<std::vector, std::vector, uint8_t, uint8_t>
    ("/home/greg/Projects/Github/mnist");
    
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
  mnist::MNIST_dataset<std::vector, std::vector<uint8_t>, uint8_t> _data;

  // data index
  std::vector<int> _training;

  // set input
  void set_input(Graph& g, std::vector<uint8_t>& image)
  {
    for (int r=0; r<28; r++)
    for (int c=0; c<28; c++)
    {
      g.set(r * 28 + c, image[r * 28 + c]);
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

