#include "client.h"
#include "server.h"

#include <csignal>
#include <cstdint>
#include <cstring>
#include <iostream>

std::unique_ptr<Server> server;
std::unique_ptr<Client> client;

void handle_signal(int32_t signal) {
  server = nullptr;
  client = nullptr;
  exit(0);
}

struct Config {
  const char *ip{kDefaultIP};
  uint16_t port{kDefaultPort};
  bool is_server{false};
  bool is_drop{true};
};

Config get_config(int32_t argc, char *const argv[]) {
  Config config;
  int32_t opt_value = 0;
  const char *opts = "sdhi:p:";

  const char *help = "Usage: channel [options]\n"
                     "Options:\n"
                     "  -h\t\tShow this help message\n"
                     "  -s\t\tRun as server\n"
                     "  -d\t\tDisable drop\n"
                     "  -i\t\tIP address\n"
                     "  -p\t\tPort number\n";

  while ((opt_value = getopt(argc, argv, opts)) != -1) {
    switch (opt_value) {
    case 's':
      config.is_server = true;
      break;
    case 'i':
      config.ip = optarg;
      break;
    case 'p':
      config.port = std::stoi(optarg);
      break;
    case 'd':
      config.is_drop = false;
      break;
    case 'h':
      printf("%s", help);
      exit(0);
      break;
    default:
      printf("%s", help);
      break;
    }
  }

  return config;
}

int32_t main(int32_t argc, char *const argv[]) {
  Config config = get_config(argc, argv);

  signal(SIGINT, handle_signal);
  signal(SIGTERM, handle_signal);

  if (config.is_server) {
    // echo "hello world" | channel -s -i 127.0.0.1 -p 8001
    // then create server
    server = std::make_unique<Server>(config.ip, config.port);
    while (true) {
      std::string message;
      std::getline(std::cin, message);
      message += "\n";
      server->send_message(message, config.is_drop);
    }
  } else {
    // channel -i 127.0.0.1 -p 8001
    // then create client
    client = std::make_unique<Client>(config.ip, config.port);
    client->recv_message();
  }

  return 0;
}