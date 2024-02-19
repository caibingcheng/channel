#ifndef CHANNEL_SERVER_H
#define CHANNEL_SERVER_H

#include <arpa/inet.h>
#include <stdint.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <condition_variable>
#include <list>
#include <mutex>
#include <string>
#include <thread>

#include "config.h"

class Server {
public:
  Server(const char *ip, uint16_t port) {
    ip_ = ip;
    port_ = port;
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    close(server_socket_);

    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) {
      throw "Failed to create server socket";
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_);
    server_address.sin_addr.s_addr = inet_addr(ip_.c_str());

    if (bind(server_socket_, (struct sockaddr *)&server_address,
             sizeof(server_address)) < 0) {
      throw "Failed to bind server socket";
    }

    if (listen(server_socket_, kMaxClientConnections) < 0) {
      throw "Failed to listen on server socket";
    }

    process_();
  }

  ~Server() {
    stop_ = true;
    messages_condition_.notify_one();
    if (server_thread_.joinable()) {
      server_thread_.join();
    }
    close(server_socket_);
    printf("Server closed\n");
  }

  void send_message(const std::string &message) {
    {
      std::lock_guard<std::mutex> lock(messages_mutex_);
      messages_.push_back(message);
      if (messages_.size() > kMaxMessageQueueSize) {
        printf("Drop message: %s\n", messages_.front().c_str());
        messages_.pop_front();
      }
    }
    messages_condition_.notify_one();
  }

private:
  void process_() {
    server_thread_ = std::thread([this]() {
      std::string message = "";
      while (!stop_) {
        int32_t client_socket = accept(server_socket_, NULL, NULL);
        if (client_socket < 0) {
          break;
        }

        char client_ip[INET_ADDRSTRLEN];
        struct sockaddr_in client_address;
        socklen_t client_address_length = sizeof(client_address);

        getpeername(client_socket, (struct sockaddr *)&client_address,
                    &client_address_length);
        inet_ntop(AF_INET, &client_address.sin_addr, client_ip,
                  INET_ADDRSTRLEN);
        printf("Client connected: %s\n", client_ip);

        while (!stop_) {
          {
            std::unique_lock<std::mutex> lock(messages_mutex_);
            messages_condition_.wait(
                lock, [this] { return !messages_.empty() || stop_; });
            if (stop_) {
              break;
            }
          }

          {
            std::lock_guard<std::mutex> lock(messages_mutex_);
            message = messages_.front();
            messages_.pop_front();
          }

          bool is_alive = true;
          do {
            uint32_t message_length = message.size();
            if (message_length > kMaxMessageSize) {
              message_length = kMaxMessageSize;
            }
            is_alive =
                (send(client_socket, &message_length, sizeof(message_length),
                      MSG_NOSIGNAL) == sizeof(message_length)) &&
                send(client_socket, message.c_str(), message_length,
                     MSG_NOSIGNAL) == message_length;
            if (!is_alive) {
              break;
            }

            message = message.substr(message_length);
          } while (message.size() != 0);

          if (!is_alive) {
            break;
          }
        }

        close(client_socket);
        printf("Client disconnected: %s\n", client_ip);
      }
    });
  }

private:
  std::string ip_;
  uint16_t port_;
  int32_t server_socket_;

  bool stop_{false};
  std::mutex messages_mutex_;
  std::condition_variable messages_condition_;
  std::thread server_thread_;

  std::list<std::string> messages_;
};

#endif // CHANNEL_SERVER_H
