#ifndef CHANNEL_CLIENT_H
#define CHANNEL_CLIENT_H

#include <arpa/inet.h>
#include <stdint.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <memory>

#include "config.h"
#include "log.h"

class Client {
 public:
  Client(const char *ip, uint16_t port) {
    ip_ = ip;
    port_ = port;
    client_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    close(client_socket_);

    client_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket_ < 0) {
      throw "Failed to create client socket\n";
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

    if (connect(client_socket_, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
      throw "Failed to connect to server\n";
    }

    const int32_t epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
      throw "Failed to create epoll\n";
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = client_socket_;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket_, &event) < 0) {
      throw "Failed to add client socket to epoll\n";
    }

    char message[kMaxMessageSize + 1];
    memset(message, 0, kMaxMessageSize + 1);
    uint64_t recv_bytes = 0;
    struct epoll_event events[1];
    while (true) {
      if (epoll_wait(epoll_fd, events, 1, -1) < 0) {
        Log::error("Failed to wait for epoll\n");
        break;
      }

      const int32_t valread = read(client_socket_, message, kMaxMessageSize);
      if (valread > 0) {
        Log::raw("%s", message);
        recv_bytes += valread;
        Log::debug("Received %lu bytes", recv_bytes);
        memset(message, 0, valread);
      }

      if (valread <= 0) {
        break;
      }
    }

    close(epoll_fd);
    close(client_socket_);
    client_socket_ = -1;
  }

 private:
  const char *ip_;
  uint16_t port_;
  int32_t client_socket_{-1};
};

#endif  // CHANNEL_CLIENT_H