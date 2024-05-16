#include "client.h"
#include "log.h"
#include "server.h"

#include <csignal>
#include <cstdint>
#include <cstring>
#include <iostream>

struct Config {
  const char *ip{kDefaultIP};
  uint16_t port{kDefaultPort};
  bool is_server{false};
  bool is_drop{true};
};

Config get_config(int32_t argc, char *const argv[]) {
  Config config;
  int32_t opt_value = 0;
  const char *opts = "sdhi:p:l:";

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
      Log::raw("%s", help);
      exit(0);
      break;
    case 'l':
      Log::set_level(std::stoi(optarg));
      break;
    default:
      Log::raw("%s", help);
      break;
    }
  }

  return config;
}

int32_t main(int32_t argc, char *const argv[]) {
  Config config = get_config(argc, argv);
  try {
    if (config.is_server) {
      Log::debug("Running as server");
      // echo "hello world" | channel -s
      std::unique_ptr<Server> server = std::make_unique<Server>(config.port);
      server->send_message(config.is_drop);
    } else {
      Log::debug("Running as client");
      // channel
      std::unique_ptr<Client> client = std::make_unique<Client>(config.ip, config.port);
      client->recv_message();
    }
  } catch (const char *message) {
    Log::raw("%s", message);
  } catch (const std::exception &e) {
    Log::raw("%s", e.what());
  } catch (...) {
    Log::raw("Unknown exception");
  }
  Log::debug("Exiting");

  return 0;
}