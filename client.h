#ifndef CHANNEL_CLIENT_H
#define CHANNEL_CLIENT_H

#include <arpa/inet.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <memory>

#include "config.h"

class Client {
public:
  Client(const char *ip, uint16_t port) {
    ip_ = ip;
    port_ = port;
    client_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    close(client_socket_);

    client_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket_ < 0) {
      throw "Failed to create client socket";
    }
  }

  ~Client() {
    if (client_socket_ > 0) {
      close(client_socket_);
    }
  }

  void recv_message() {
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_);
    server_address.sin_addr.s_addr = inet_addr(ip_);

    if (connect(client_socket_, (struct sockaddr *)&server_address,
                sizeof(server_address)) < 0) {
      throw "Failed to connect to server";
    }

    char message[kMaxMessageSize] = {0};
    uint32_t message_length = 0;
    while (true) {
      // send alive message
      if (write(client_socket_, &message_length, sizeof(message_length)) !=
          sizeof(message_length)) {
        break;
      }

      if (read(client_socket_, &message_length, sizeof(message_length)) !=
          sizeof(message_length)) {
        break;
      }

      // read line by line
      int32_t valread = read(client_socket_, message, message_length);
      if (valread > 0) {
        printf("%s", message);
      }
      memset(message, 0, kMaxMessageSize);

      if (valread == 0) {
        break;
      }
    }

    close(client_socket_);
  }

private:
  const char *ip_;
  uint16_t port_;
  int32_t client_socket_{-1};
};

#endif // CHANNEL_CLIENT_H