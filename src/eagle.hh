/**
 * Copyright (c) 2019 Greg Padiasek
 * Distributed under the terms of the the 3-Clause BSD License.
 * See the accompanying file LICENSE or the copy at 
 * https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef _EAGLE_H_
#define _EAGLE_H_

#include <math.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <algorithm>

#include <limits.h>
#include "random.hh"

// used node types (must be consecutive numbers)
#define NODE_INPUT    1
#define NODE_ADD      2
#define NODE_MUL      3

// node types that can be added or removed
#define NODE_MINIMUM  1
#define NODE_MAXIMUM  3

// computation precision type
#define DTYPE float

// parameter compression level
#define DTYPE_PRECISION 1e-3

// differential delta (lower is more accurate)
#define FINITE_DELTA 1e-4

// reinforcement learning reward discount
#define GAMMA_DISCOUNT 0.99

// learning rate (lower is slower but more stable)
#define LEARNING_RATE 1e-2

// mutation probability per byte
#define MUTATION_PROB 1e-3

// node base class
class Node
{
public:
  // ctor
  Node(RNG& rng) : _rng(rng)
  {
    _cache = true;
    _bias = 1;
    _bgrad = 0;
  }
  
  // dtor
  virtual ~Node() {}

  // node type
  virtual int type() const = 0;
  
  // state S at time t (-1 is for current time)
  virtual DTYPE S(int t = -1) const = 0;

  // analog state derivative dS/dw w.r.t. i-th weight at time t
  virtual DTYPE dSdw(int i, int t = -1) const = 0;

  // analog state derivative dS/db w.r.t. bias at time t
  virtual DTYPE dSdb(int t = -1) const = 0;

  // activation at time t
  virtual DTYPE A(int t = -1) const { return P(t) > _rng.uniform_dec(0, 1); }
    
  // reset state but keep the gradients
  void reset()
  { 
    _cache = false;
    _state.clear();
    _output.clear();
    _reward.clear();
  }

  // reset cache
  void recache()
  {
    _cache = false;
  }
  
  // set bias
  void set_bias(DTYPE bias)
  {
    _bias = bias;
  }

  // get bias
  DTYPE get_bias() const
  {
    return _bias;
  }
    
  // compute output (t == -1 is for current time)
  DTYPE output(int t = -1)
  {
    if (t >= 0) return _output[t];
    if (!_cache)
    {      
      // set temp values
      _state.push_back(0);
      _output.push_back(0);
      _cache = true;
      // update values
      _state.back() = S();
      _output.back() = A();
    }
    return _output.back();
  }

  // create input connection
  void insert(Node* node, DTYPE weight = 1)
  {
    _cache = false;
    _input.push_back(node);
    _weight.push_back(weight);
    _wgrad.push_back(0);
  }

  // accumulate rewards
  void reward(DTYPE reward)
  {
    // only collect reward on reachable nodes (_state is updated)
    if (_state.size() > _reward.size()) _reward.push_back(reward);
  }

  // accumulate gradients
  void gradient(DTYPE gamma = GAMMA_DISCOUNT)
  {
    auto rsize = _reward.size();
    auto isize = _input.size();
  
    // get discounted rewards
    DTYPE r = 0;
    std::vector<DTYPE> reward(_reward.size());
    for (int i=rsize-1; i>=0; i--)
    {
      r = gamma * r  + _reward[i];
      reward[i] = r;
    }

    // cache the state derivatives
    std::vector<DTYPE> dlds(_reward.size());
    for (int t=0; t<rsize; t++) dlds[t] = dLdS(reward[t], t);

    // update gradient
    for (int t=0; t<rsize; t++)
    {
      //dL/dw
      for (int i=0; i<isize; i++) _wgrad[i] += dlds[t] * dSdw(i, t);
      //dL/db
      _bgrad += dlds[t] * dSdb(t);
    }
  }
  
  // apply gradients
  void update(DTYPE lr = LEARNING_RATE)
  {
    auto size = _input.size();

    //w = w - rate * dL/dw
    for (int i=0; i<size; i++) _weight[i] -= lr * _wgrad[i];
    //b = b - rate * dL/db
    _bias -= lr * _bgrad;

    // reset gradients
    _wgrad.assign(_wgrad.size(), 0);
    _bgrad = 0;
  }

  // policy P to derive dLdS
  DTYPE P(int t = -1) const
  {
    auto state = (t == -1) ? _state.back() : _state[t];
    return sigmoid(state);
  }
  
  // loss L at time t to derive dLdS
  DTYPE L(DTYPE reward, int t = -1) const
  {
    if (type() == NODE_INPUT) return 0;
    // REINFORCE: E[log(P(x)) * r(x)],
    // action 0 loss: -log(1-P) * r
    // action 1 loss: -log(P) * r
    auto p = P(t);
    auto a = (t == -1) ? _output.back() : _output[t];
    return -logf((1 - a) * (1 - p) + a * p) * reward;
  }

  // loss derivative w.r.t. state at time t
  DTYPE dLdS(DTYPE reward, int t = -1) const
  {
    if (type() == NODE_INPUT) return 0;
    auto state = (t == -1) ? _state.back() : _state[t];
    auto active = (t == -1) ? _output.back() : _output[t];
    auto exp_s = expf(state);
    auto sign = (1 - 2 * active);
    return sign * reward * sigmoid(sign * state);
  }
  
  // loss derivative dS/dw w.r.t. i-th weight
  DTYPE dLdw(int i, DTYPE reward, int t = -1) const
  {
    return dLdS(reward, t) * dSdw(i, t);
  }

  // loss derivative dS/dw w.r.t. bias
  DTYPE dLdb(DTYPE reward, int t = -1) const
  {
    return dLdS(reward, t) * dSdb(t);
  }

  // numerical loss derivative dL/dw
  DTYPE dLdw_numeric(int i, DTYPE reward, int t = -1)
  {
    return dLdS(reward, t) * dSdw_numeric(i, t);
  }

  // numerical loss derivative dL/db
  DTYPE dLdb_numeric(DTYPE reward, int t = -1)
  {
    return dLdS(reward, t) * dSdb_numeric(t);
  }

  // numerical state derivative dS/dw
  DTYPE dSdw_numeric(int i, int t)
  {
    DTYPE w = _weight[i];
    _weight[i] = w - FINITE_DELTA;
    DTYPE S1 = S(t);
    _weight[i] = w + FINITE_DELTA;
    DTYPE S2 = S(t);
    _weight[i] = w;
    return (S2 - S1)/FINITE_DELTA/2;
  }

  // numerical state derivative dS/db
  DTYPE dSdb_numeric(int t)
  {
    DTYPE w = _bias;
    _bias = w - FINITE_DELTA;
    DTYPE S1 = S(t);
    _bias = w + FINITE_DELTA;
    DTYPE S2 = S(t);
    _bias = w;
    return (S2 - S1)/FINITE_DELTA/2;
  }

  // sigmoid activation
  static DTYPE sigmoid(DTYPE x)
  {
    return 1.f / (1.f + expf(-x));
  }

  // node data
  std::vector<Node*> _input;  // input connections
  std::vector<DTYPE> _weight; // input weights
  std::vector<DTYPE> _wgrad; // weights gradients
  std::vector<DTYPE> _state;  // state history
  std::vector<DTYPE> _output;  // output history
  std::vector<DTYPE> _reward; // reward history
  DTYPE _bias;
  DTYPE _bgrad;
  bool _cache;
  RNG& _rng;
};

// special type of node to keep track input data
class Input: public Node
{
public:
  Input(RNG& rng) : Node(rng) {}
  void set(DTYPE value)
  { 
    _cache = false;
    _value = value;
  }
  virtual int type() const { return NODE_INPUT; }
  virtual DTYPE S(int t = -1) const
  { 
    return (t == -1) ? _value : _state[t];
  }
  virtual DTYPE dSdw(int i, int t = -1) const { return 0; }
  virtual DTYPE dSdb(int t = -1) const { return 0; }
  virtual DTYPE A(int t = -1) const { return S(); }
private:
  DTYPE _value;
};

// addition opertation
class Add: public Node
{
public:
  Add(RNG& rng) : Node(rng) {}
  virtual int type() const { return NODE_ADD; }

  // state at time t
  virtual DTYPE S(int t = -1) const
  {
    DTYPE state = _bias; // bias
    auto size = _input.size();
    for (int i=0; i<size; i++) state += _weight[i] * _input[i]->output(t);
    return state;
  }
    
  // state derivative w.r.t. i-th weight at time t
  virtual DTYPE dSdw(int i, int t = -1) const
  {
    return _input[i]->output(t); // weight gradient
  }

  // state derivative w.r.t. bias at time t
  virtual DTYPE dSdb(int t = -1) const
  {
    return 1; // bias gradient
  }
};

// gating/multiplication opertation
class Mul: public Node
{
public:
  Mul(RNG& rng) : Node(rng) {}
  virtual int type() const { return NODE_MUL; }

  // state at time t
  virtual DTYPE S(int t = -1) const
  {
    auto size = _input.size();
    DTYPE state = _bias; // bias
    for (int i=0; i<size; i++) state *= (_weight[i] + _input[i]->output(t));
    return state;
  }
    
  // state derivative w.r.t. i-th weight at time t
  virtual DTYPE dSdw(int i, int t = -1) const
  {
    DTYPE state = _bias; // bias
    int size = _input.size();
    for (int j=0; j<size; j++)
    {
      if (j == i) continue;
      state *= (_input[j]->output(t) + _weight[j]);
    }
    
    return state;
  }

  // state derivative w.r.t. bias at time t
  virtual DTYPE dSdb(int t = -1) const
  {
    DTYPE state = 1;
    int size = _input.size();    
    for (int j=0; j<size; j++)
    {
      state *= (_input[j]->output(t) + _weight[j]);
    }    
    return state;
  }
};

// computational graph
class Graph
{
public:
  Graph(int input, int output, int mx_hidden, int mx_links, RNG& rng) : _rng(rng)
  {
    _meta.input = input;
    _meta.output = output;
    _meta.hidden = mx_hidden;
    _meta.links = mx_links;

    for (auto i=0; i<_meta.input; i++)
    {
      _nodes.push_back(new Input(_rng));
      _nodes_index.push_back(i);
      _links_index.emplace_back();
    }
    for (auto i=0; i<_meta.output; i++)
    {
      _nodes.push_back(new_node());
      _nodes_index.push_back(_meta.input + i);
      _links_index.emplace_back();
    }    
  }

  ~Graph()
  {
    clear();
  }

  // number of graph connections
  uint32_t size() const
  { 
    int size = 0;
    for (auto e: _nodes) size += e->_input.size();
    return size;
  }

  // maximum number of graph connections
  uint32_t capacity() const
  { 
    return (_meta.hidden + _meta.output) * _meta.links;
  }
  
  void clear()
  {
    for (auto e: _nodes) delete e;
    _nodes.clear();
    _nodes_index.clear();
    _links_index.clear();
  }

  void set(uint32_t input, DTYPE value)
  {
    ((Input*)_nodes[input])->set(value);
  }

  DTYPE get(uint32_t output)
  {
    return _nodes[_meta.input + output]->output();
  }

  void reset()
  {
    for (auto e: _nodes) e->reset();
  }

  void recache()
  {
    for (auto e: _nodes) e->recache();
  }
  
  void reward(DTYPE reward)
  {
    for (auto e: _nodes) e->reward(reward);
  }

  void gradient(DTYPE gamma = GAMMA_DISCOUNT)
  {
    for (auto e: _nodes) e->gradient(gamma);
  }

  void update(DTYPE lr = LEARNING_RATE)
  {
    for (auto e: _nodes) e->update(lr);
  }
    
  const std::string& save()
  {
    // resize dna buffer
    auto size = link_offset(_meta, _meta.input + _meta.output + _meta.hidden, 0);
    if (_dna.size() != size) _dna.resize(size);
    char* data = &_dna[0];

    // set header
    *(MetaData*)data = _meta;
    
    // input (not saved)
    std::unordered_map<Node*, uint32_t> node_map; // node ptr to rt-index
    for (auto i=0; i<_meta.input; i++)
    {
      auto node_p = _nodes[i];
      node_map.emplace(node_p, i);
    }

    // nodes to save
    auto nodes_size = _nodes.size();
    
    // nodes (hidden + output)
    for (auto i=_meta.input; i<nodes_size; i++)
    {
      auto node_p = _nodes[i];
      node_map.emplace(node_p, i);
    
      auto offset = node_offset(_meta, _nodes_index[i]);
      NodeData &node = *(NodeData*)(data + offset);

      node.type = node_p->type();
      node.bias = to_int(node_p->get_bias());
    }
      
    // links (no links in inputs)
    for (auto i=_meta.input; i<nodes_size; i++)
    {
      auto node_p = _nodes[i];
      auto& links_index = _links_index[i];
      auto links_size = links_index.size();
      for (auto j=0; j<links_size; j++)
      {
        auto offset = link_offset(_meta, _nodes_index[i], links_index[j]);
        LinkData &link = *(LinkData*)(data + offset);

        auto source = node_map[node_p->_input[j]];
        link.source = _nodes_index[source];
        link.weight = to_int(node_p->_weight[j]);
      }
    }
    
    return _dna;
  }

  bool load(const std::string& in)
  {
    clear();
        
    // get data buffer
    if (in.size() < sizeof(MetaData)) return false;
    const char* data = in.data();
    
    // load graph according to stored meta data
    MetaData &meta = *(MetaData*)data;

    // validate data buffer
    auto max_nodes = meta.input + meta.output + meta.hidden;
    if (in.size() < link_offset(meta, max_nodes, 0)) return false;
    _dna = in;
    
    // inputs (not saved)
    std::unordered_map<uint32_t, uint32_t> node_map; // store to rt node index
    for (auto i=0; i<meta.input; i++)
    {
      // add input
      node_map.emplace(i, _nodes.size());
      _nodes_index.push_back(i);
      _links_index.emplace_back();
      _nodes.push_back(new Input(_rng));
    }
            
    // nodes (hidden + output)
    for (auto i=meta.input; i<max_nodes; i++)
    {
      // read node
      auto offset = node_offset(meta, i);
      NodeData &node = *(NodeData*)(data + offset);
      
      // validate node (accept 0:NODE_MAXIMUM, 0 is inactive)
      auto node_ptr = new_node(node.type % (NODE_MAXIMUM + 1));
      //auto node_ptr = new_node(node.type);
      if (node_ptr == nullptr) continue;
      node_ptr->set_bias(to_dec(node.bias));
 
      // add node
      node_map.emplace(i, _nodes.size());
      _nodes_index.push_back(i);
      _links_index.emplace_back();
      _nodes.push_back(node_ptr);
    }
    
    // node links (no links in inputs)
    for (auto i=meta.input; i<max_nodes; i++)
    {
      // validate target node
      auto target_it = node_map.find(i);
      if (target_it == node_map.end()) continue;

      for (auto j=0; j<meta.links; j++)
      {
        // read link
        auto offset = link_offset(meta, i, j);
        LinkData &link = *(LinkData*)(data + offset);

        // validate source node (accept 0:max_nodes, max_nodes is inactive)
        auto source_it = node_map.find(link.source % (max_nodes + 1));
        //auto source_it = node_map.find(link.source);
        if (source_it == node_map.end()) continue;
        
        // create link
        auto source = (*source_it).second;
        auto target = (*target_it).second;
        _nodes[target]->insert(_nodes[source], to_dec(link.weight));
        _links_index[target].push_back(j);
      }
    }

    // update graph size
    _meta.input  = std::max(_meta.input,  meta.input);
    _meta.output = std::max(_meta.output, meta.output);
    _meta.hidden = std::max(_meta.hidden, meta.hidden);
    _meta.links  = std::max(_meta.links,  meta.links);
        
    // validate graph
    return is_valid();
  }
  
  bool is_valid()
  {
    auto nodes_size = _nodes.size();
    if (nodes_size < _meta.input + _meta.output) return false;
    for (auto i=0; i<_meta.input; i++)
    {
      if (_nodes[i]->type() != NODE_INPUT) return false;
    }
    for (auto i=_meta.input; i<nodes_size; i++)
    {
      if (_nodes[i]->type() == NODE_INPUT) return false;
    }
    return true;
  }

  Graph* crossover(Graph& other, DTYPE mut_prob = MUTATION_PROB)
  {
    // update mutable arrays
    auto& A = save();
    auto& B = other.save();
    
    // validate size
    if (A.size() != B.size()) return  nullptr;

    // randomize order
    auto order = _rng.uniform_int(1);
    const std::string* pa = (order) ? &A : &B;
    const std::string* pb = (order) ? &B : &A;

    // 1-point crossover
    auto offset = sizeof(MetaData);
    auto index = _rng.uniform_int(offset, A.size()-1);
    auto C = pa->substr(0, index) + pb->substr(index);
    
    // set mutation level
    auto dna = &C[0];
    MetaData& meta = *(MetaData*)dna;
    
    // get A and B bytes
    uint8_t a = (*pa)[index];
    uint8_t b = (*pb)[index];
    auto bits = _rng.uniform_int(8); // 0-8 shared bits

    // drop lower bits
    a >>= 8-bits;
    a <<= 8-bits;

    // drop higher bits
    b <<= bits;
    b >>= bits;

    // crossover higher and lower bits
    dna[index] = a | b;
    
    // random mutation on each byte
    for (int i=C.size()-1; i>=offset; i--)
    if (_rng.uniform_dec(1.0) < mut_prob) dna[i] ^= (1 << _rng.uniform_int(7));

    // create instance
    auto g = new Graph(0,0,0,0,_rng);
    if (g->load(C) == false)
    {
      delete g;
      g = nullptr;
    }

    return g;
  }

  struct MetaData
  {
    uint32_t input;  // input size
    uint32_t output; // output size
    uint32_t hidden; // max hidden nodes
    uint32_t links;  // max input links per node
  };
  
  struct NodeData
  {
    uint32_t type; // node type
    uint32_t bias; // node bias
  };

  struct LinkData
  {
    uint32_t source; // link source
    uint32_t weight; // link weight
  };

  int32_t to_int(float f) const
  {
    f /= DTYPE_PRECISION;
    f = (f > INT_MAX) ? INT_MAX : f;
    return (f < INT_MIN) ? INT_MIN : f;
  }

  float to_dec(int32_t i) const
  {
    return i * DTYPE_PRECISION;
  }
  
  uint32_t node_offset(const MetaData& meta, uint32_t node) const
  {
    // input has virtual index and is not present in store
    return sizeof(MetaData) + (node - meta.input) * sizeof(NodeData);
  }

  uint32_t link_offset(const MetaData& meta, uint32_t node, uint32_t link) const
  {
    // input has virtual index and is not present in store
    return sizeof(MetaData)
        + (meta.output + meta.hidden) * sizeof(NodeData)
        + ((node - meta.input) * meta.links + link) * sizeof(LinkData);
  }
  
  // create specific node when type != -1, or random node when type = -1,
  // NODE_INPUT is excluded in random type selection mode (type = -1)
  Node* new_node(int type = -1)
  {
    if (type == -1) type = _rng.uniform_int(NODE_MINIMUM+1, NODE_MAXIMUM);
    Node* node = nullptr;
    switch(type)
    {
      case NODE_INPUT: node = new Input(_rng); break;
      case NODE_ADD: node = new Add(_rng); break;
      case NODE_MUL: node = new Mul(_rng); break;
    }
    return node;
  }
  
  std::string _dna; // mutable container of *ALL* genes/features
  std::vector<Node*> _nodes; // [input..., output..., hidden...]
  std::vector<uint32_t> _nodes_index; // nodes store index
  std::vector<std::vector<uint32_t>> _links_index; // links store index
  MetaData _meta;
  RNG& _rng;
};

class NeuroEvolution
{
public:
  NeuroEvolution(int input, int output, int max_hidden, int max_links, int size)
  {
    _epoch = 1000;
    _objective = 0.0;
    size = std::max(4, (size/2)*2);

    for (auto i=0; i<size; i++)
    {
      _population.emplace_back(NAN, 
      new Graph(input, output, max_hidden, max_links, _rng));
    }
  }

  virtual ~NeuroEvolution()
  {
    for (auto& e: _population) delete e.second;
  }

  void seed(const std::string& graph)
  {
    _population.back().second->load(graph);
    _rng.seed();
  }
  
  std::string best() 
  {
    return _population.front().second->save();
  }

  DTYPE fitness() { return _population.front().first; }
  
  DTYPE objective() { return _objective; }

  void run()
  {
    // probability distribution for crossover selection
    auto size = _population.size();
    std::vector<int> crossover(size/2);
    std::vector<Graph*> offspring(size/2);
    for (auto i=0; i<size/2; i++) crossover[i] = size/2 - i;
    
    // run epoch
    for (auto s=0; s<_epoch; s++)
    {
      // evaluate all elements
      for (auto& e: _population) e.first = episode(*e.second);
      
      // sort population by rewards in descending order
      std::sort(_population.rbegin(), _population.rend());

      // create new generation offspring from entire population
      for (auto i=0; i<size/2; i++)
      {
        auto M = _rng.discrete_choice(crossover.begin(), crossover.end());
        auto F = _rng.discrete_choice(crossover.begin(), crossover.end());
        auto male = _population[2*M].second;
        auto female = _population[2*F + 1].second;
        offspring[i] = male->crossover(*female);
      }

      // replace the weak half of the population with the new offspring
      for (auto i=0; i<size/2; i++)
      {
        if (offspring[i] != nullptr)
        {
          delete _population[size - i - 1].second;
          _population[size - i - 1].second = offspring[i];
        }
      }
    }

    // calculate dna capacity
    auto capacity = (float) _population.front().second->size() / 
                    _population.front().second->capacity();

    // increase search space
    if (capacity > 0.5)
    {
      auto meta = _population.front().second->_meta;
      meta.hidden *= 2; 
      meta.links *= 2; 
      for (auto& e: _population) e.second->_meta = meta;
    }
  }

protected:
  // train episode that updates graph weights and returns graph reward
  virtual DTYPE episode(Graph& g) = 0;
  
protected:
  RNG _rng;
  uint32_t _epoch;
  DTYPE _objective;
  std::vector<std::pair<DTYPE, Graph*>> _population;
};

#endif /*_EAGLE_H_*/
