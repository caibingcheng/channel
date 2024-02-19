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

void get_config(const char **ip, uint16_t *port, bool *is_server, int32_t argc,
                char *argv[]) {
  if (argc > 1) {
    if (strcmp(argv[1], "server") == 0) {
      if (argc > 2) {
        *ip = argv[2];
      }
      if (argc > 3) {
        *port = std::stoi(argv[3]);
      }
      *is_server = true;
    } else {
      if (argc > 1) {
        *ip = argv[1];
      }
      if (argc > 2) {
        *port = std::stoi(argv[2]);
      }
    }
  }
}

int main(int32_t argc, char *argv[]) {
  const char *ip = kDefaultIP;
  uint16_t port = kDefaultPort;
  bool is_server = false;
  get_config(&ip, &port, &is_server, argc, argv);

  signal(SIGINT, handle_signal);
  signal(SIGTERM, handle_signal);

  if (is_server) {
    // echo "hello world" | channel server 127.0.0.1 8001
    // then create server
    server = std::make_unique<Server>(ip, port);
    while (true) {
      std::string message;
      std::getline(std::cin, message);
      message += "\n";
      server->send_message(message);
    }
  } else {
    // channel 127.0.0.1 8001
    // then create client
    client = std::make_unique<Client>(ip, port);
    client->recv_message();
  }

  return 0;
}