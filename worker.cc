/**
 * Copyright (c) 2019 Greg Padiasek
 * Distributed under the terms of the MIT License.
 * See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT
 */

#include <iostream>
#include <chrono>
#include <thread>

#include <dlfcn.h>

#include "transport.hh"
#include "eagle.pb.h"
#include "eagle.hh"

// worker runtime data

bool done = false;
std::string host;
int port = -1;

typedef NeuroEvolution* (*create_callback)();
typedef void (*destroy_callback)(NeuroEvolution*);

create_callback create = nullptr;
destroy_callback destroy = nullptr;

// command handlers

DTYPE get_fitness()
{
  eagle::Request req;
  eagle::Response res;

  auto get_fitness = req.mutable_get_fitness();  

  ProtobufClient<eagle::Request, eagle::Response> client;
  client.connect(host, port);  
  client.send(req);    
  client.receive(res);
  client.disconnect();
  
  if (res.has_error()) throw std::runtime_error(res.error().message());
  
  return res.get_fitness().fitness();
}

std::string get_graph(DTYPE fitness)
{
  eagle::Request req;
  eagle::Response res;

  auto get_graph = req.mutable_get_graph();

  ProtobufClient<eagle::Request, eagle::Response> client;    
  client.connect(host, port);    
  client.send(req);    
  client.receive(res);
  client.disconnect();
  
  if (res.has_error()) throw std::runtime_error(res.error().message());
  
  auto master_fitness = res.get_graph().fitness();
  auto master_graph = res.get_graph().graph();
  
  std::cout << "thread " << std::this_thread::get_id()
            << ", recv size " << master_graph.size()
            << ", fitness " << master_fitness
            << " (" << fitness << ")" << std::endl;

  return master_graph;
}

void set_graph(const std::string& graph, DTYPE fitness, DTYPE master_fitness)
{
  eagle::Request req;
  eagle::Response res;

  auto set_graph = req.mutable_set_graph();
  set_graph->set_fitness(fitness);
  set_graph->set_graph(graph);

  ProtobufClient<eagle::Request, eagle::Response> client;    
  client.connect(host, port);
  client.send(req);    
  client.receive(res);
  client.disconnect();
  
  if (res.has_error()) throw std::runtime_error(res.error().message());
  
  std::cout << "thread " << std::this_thread::get_id()
            << ", send size " << graph.size()
            << ", fitness " << master_fitness
            << " (" << fitness << ")" << std::endl;
}

// worker routines

void thread_run()
{
  RNG rng;
  NeuroEvolution& impl = *create();

  while (!done)
  {
    // worker fitness
    auto fitness = impl.fitness();
    
    // master fitness
    auto master_fitness = get_fitness();
    
    // stop if desiret accuracy was reached
    if (master_fitness >= impl.objective()) break;
    
    // validate fitness
    bool master_nan = std::isnan(master_fitness);
    bool worker_nan = std::isnan(fitness);
    
    // get master graph to local population
    if (master_fitness > fitness || (!master_nan && worker_nan))
    {
      impl.seed(get_graph(fitness));
    }
    else
    // send localy fit graph to master
    if (master_fitness < fitness || (master_nan && !worker_nan))
    {
      set_graph(impl.best(), fitness, master_fitness);
    }

    // evolve the graph
    impl.run();
  }

  destroy(&impl);
}

void worker_run(const std::string& impl, const std::string& host, int port)
{  
  void* handle = dlopen(impl.c_str(), RTLD_LAZY);
  if (handle == nullptr)
  {
    std::ostringstream log;
    log << "Failed to load libary '" << impl << "'";
    throw std::runtime_error(log.str());
  }
  
  ::create = (create_callback)dlsym(handle, "create");
  if (create == nullptr)
    throw std::runtime_error("Failed to locate symbol 'create'");
  ::destroy = (destroy_callback)dlsym(handle, "destroy");
  if (destroy == nullptr)
    throw std::runtime_error("Failed to locate symbol 'destroy'");

  ::host = host;
  ::port = port;

  std::vector<std::thread> pool;

  int threads = std::thread::hardware_concurrency();
  std::cout << "starting " << threads << " threads..." << std::endl;

  for (int i=0; i<threads; i++) pool.emplace_back(thread_run);

  for (auto& e: pool) e.join();
  
  dlclose(handle);
}

void worker_term()
{
  done = true;
}

