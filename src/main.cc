/**
 * Copyright (c) 2019 Greg Padiasek
 * Distributed under the terms of the the 3-Clause BSD License.
 * See the accompanying file LICENSE or the copy at 
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <iostream>
#include <thread>
#include <csignal>

#include "transport.hh"
#include "eagle.pb.h"

// master routines
extern void master_init(const std::string& file);
extern void master_run(const ServerContext& ctx,
eagle::Request& req, eagle::Response& res);
extern void master_err(const std::exception& err, eagle::Response& res);
extern void master_term();

// worker routines
extern void worker_run(const std::string& library,
const std::string& host, int port);
extern void worker_term();

// term routine
typedef void (*v_routine)();
v_routine term_routine = nullptr;

// server type
typedef ProtobufServer<eagle::Request, eagle::Response> GraphServer;

// server pointer for singal handling
std::shared_ptr<GraphServer> eagle_server;

// signal handler
static void on_signal(int signum) {
  if (eagle_server != nullptr) eagle_server->stop();
  else 
  if (term_routine !=nullptr) term_routine();
}

// syntax message
void syntax(char* argv[]) {
  std::cerr << "Usage: " << argv[0] << " "
            << "master <FILE> <PORT> | "
            << "worker <HOST> <PORT> <IMPL>"
            << std::endl;
}

/**
 * server entry point
 */
int main(int argc, char* argv[]) {

  try {
    if (argc < 2) {
      syntax(argv);
      return 1;
    }

    // get server role
    std::string role = argv[1];

    // set signal handler
    signal(SIGINT, on_signal);

    if (role == "master") {
      if (argc != 4) {
        syntax(argv);
        return 1;
      }
      
      std::string file = argv[2];
      int port = std::stoi(argv[3]);
      std::cout << "Starting " << role << " on port " << port << std::endl;

      // start master
      master_init(file);
      eagle_server = std::make_shared<GraphServer>(master_run, master_err);
      eagle_server->run(port, std::thread::hardware_concurrency());
      master_term();

      std::cout << "Stopping " << role << " on port " << port << std::endl;
    }
    else
    if (role == "worker") {
      if (argc != 5) {
        syntax(argv);
        return 1;
      }

      std::string host = argv[2];
      int port = std::stoi(argv[3]);
      std::string impl = argv[4];
      std::cout << "Starting " << role << " at " 
                << host << ":" << port << std::endl;

      // start worker
      term_routine = worker_term;
      worker_run(impl, host, port);

      std::cout << "Stopping " << role << " at " 
                << host << ":" << port << std::endl;
    }
    else {
      std::cerr << "Unknown role '" << role << "'" << std::endl;
      return 3;
    }
  }
  catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 4;
  }

  return 0;
}

