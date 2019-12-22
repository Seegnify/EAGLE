/**
 * Copyright (c) 2019 Greg Padiasek
 * Distributed under the terms of the MIT License.
 * See accompanying file LICENSE or copy at http://opensource.org/licenses/MIT
 */

#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>

#include "transport.hh"
#include "eagle.pb.h"
#include "eagle.hh"

// master runtime data

std::atomic<float> master_fitness(NAN);
std::string master_graph;
std::mutex master_lock;
std::string master_file;

// master command handlers

void log_graph(const std::string& graph, float fitness)
{
  auto time = dl::time_to_string(dl::time_now());
  std::cout << time << ", size " << graph.size()
            << ", fitness " << fitness << std::endl;
}

void save_graph(const std::string& graph, float fitness, const std::string& file)
{
  std::stringstream data;

  short version = 1;
  data.write((char*)&version, sizeof(version));

  data.write((char*)&fitness, sizeof(fitness));

  int graph_size = graph.size();
  data.write((char*)&graph_size, sizeof(graph_size));

  auto graph_data = graph.data();
  data.write((char*)graph_data, graph_size);

  dl::write_file(data, file);
}

void on_get_fitness(
const eagle::GetFitness& req, eagle::Response& res)
{
  auto response = res.mutable_get_fitness();
  response->set_fitness(master_fitness);
}

void on_get_graph(
const eagle::GetGraph& req, eagle::Response& res)
{
  std::lock_guard<std::mutex> lock(master_lock);
  auto response = res.mutable_get_graph();    
  response->set_fitness(master_fitness);
  response->set_graph(master_graph);
}

void on_set_graph(
const eagle::SetGraph& req, eagle::Response& res)
{
  auto worker_fitness = req.fitness();
  auto response = res.mutable_success();
  
  // accept the same or higher fitness
  if (worker_fitness < master_fitness) return;
  
  std::lock_guard<std::mutex> lock(master_lock);
  master_fitness = worker_fitness;
  master_graph = req.graph();
  
  save_graph(master_graph, master_fitness, master_file);
  log_graph(master_graph, master_fitness);
}

// master routines

void master_init(const std::string& file)
{
  master_file = file;

  try
  {
    std::stringstream data;
    dl::read_file(master_file, data);

    short version = 0;
    data.read((char*)&version, sizeof(version));

    if (version != 1)
    {
      std::ostringstream error;
      error << "Unsupported file version " << version;
      throw std::runtime_error(error.str());
    }

    float graph_fitness = NAN;
    data.read((char*)&graph_fitness, sizeof(graph_fitness));

    int graph_size = 0;
    data.read((char*)&graph_size, sizeof(graph_size));

    std::string graph_data(graph_size, 0);
    data.read((char*)graph_data.data(), graph_size);

    std::lock_guard<std::mutex> lock(master_lock);
    master_fitness = graph_fitness;
    master_graph = graph_data;

    log_graph(master_graph, master_fitness);
  }
  catch(std::exception& e)
  {
    std::cout << e.what() << std::endl;

    std::lock_guard<std::mutex> lock(master_lock);
    log_graph(master_graph, master_fitness);
  }
}

void master_run(const ServerContext& ctx,
eagle::Request& req, eagle::Response& res)
{
  if (req.has_get_fitness()) on_get_fitness(req.get_fitness(), res);
  else
  if (req.has_get_graph()) on_get_graph(req.get_graph(), res);
  else
  if (req.has_set_graph()) on_set_graph(req.set_graph(), res);
  else
  {
    auto error = res.mutable_error();
    error->set_status(400);
    error->set_message("Command Not Supported");
  }
}

void master_err(const std::exception& err, eagle::Response& res)
{
  auto* errptr = res.mutable_error();
  errptr->set_status(400);
  errptr->set_message(err.what());
}

void master_term()
{
  if (std::isnan(master_fitness))
  {
    std::cout << "no state saved" << std::endl;
  }
  else
  {
    std::cout << "last state saved in " << master_file << std::endl;
  }
}

