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
#include <unordered_set>

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
    clients_condition_.notify_one();

    if (client_thread_.joinable()) {
      client_thread_.join();
    }
    {
      std::lock_guard<std::mutex> lock(clients_mutex_);
      for (int32_t client_socket : client_sockets_) {
        close(client_socket);
      }
    }

    if (server_thread_.joinable()) {
      server_thread_.join();
    }
    close(server_socket_);

    printf("Server closed\n");
  }

  void send_message(const std::string &message, bool is_drop = true) {
    {
      std::lock_guard<std::mutex> lock(messages_mutex_);
      messages_.push_back(message);
      if (is_drop && (messages_.size() > kMaxMessageQueueSize)) {
        printf("Drop message: %s", messages_.front().c_str());
        messages_.pop_front();
      }
    }
    messages_condition_.notify_one();
  }

private:
  void process_() {
    client_thread_ = std::thread([this]() {
      // server socket select
      fd_set readfds;
      struct timeval timeout;
      timeout.tv_sec = 0;
      timeout.tv_usec = 1000;

      while (!stop_) {
        FD_ZERO(&readfds);
        FD_SET(server_socket_, &readfds);
        if (select(server_socket_ + 1, &readfds, NULL, NULL, &timeout) < 0) {
          continue;
        }

        if (FD_ISSET(server_socket_, &readfds)) {
          int32_t client_socket = accept(server_socket_, NULL, NULL);
          if (client_socket < 0) {
            break;
          }
          {
            std::lock_guard<std::mutex> lock(clients_mutex_);
            client_sockets_.insert(client_socket);
          }
          clients_condition_.notify_one();
          printf("New client connected, socket: %d, client count: %lu\n",
                 client_socket, client_sockets_.size());
        }
      }
    });

    server_thread_ = std::thread([this]() {
      std::string message = "";
      while (!stop_) {
        {
          std::unique_lock<std::mutex> lock(clients_mutex_);
          clients_condition_.wait(
              lock, [this] { return !client_sockets_.empty() || stop_; });
          if (stop_) {
            break;
          }
        }

        {
          std::unique_lock<std::mutex> lock(messages_mutex_);
          messages_condition_.wait(
              lock, [this] { return !messages_.empty() || stop_; });
          if (stop_) {
            break;
          }
        }

        {
          message = "";
          std::lock_guard<std::mutex> lock(messages_mutex_);
          while (message.size() < kMaxMessageSize && !messages_.empty()) {
            message += messages_.front();
            messages_.pop_front();
          }
        }

        std::list<int32_t> client_sockets;
        {
          std::lock_guard<std::mutex> lock(clients_mutex_);
          for (int32_t client_socket : client_sockets_) {
            client_sockets.push_back(client_socket);
          }
        }

        do {
          uint32_t message_length = message.size();
          if (message_length > kMaxMessageSize) {
            message_length = kMaxMessageSize;
          }

          for (auto it = client_sockets.begin(); it != client_sockets.end();) {
            const int32_t client_socket = *it;
            const bool is_alive =
                (send(client_socket, &message_length, sizeof(message_length),
                      MSG_NOSIGNAL) == sizeof(message_length)) &&
                send(client_socket, message.c_str(), message_length,
                     MSG_NOSIGNAL) == message_length;
            if (!is_alive) {
              close(client_socket);
              {
                std::lock_guard<std::mutex> lock(clients_mutex_);
                client_sockets_.erase(client_socket);
              }
              it = client_sockets.erase(it);
              printf("Client disconnected, socket: %d, client count: %lu\n",
                     client_socket, client_sockets.size());
            } else {
              ++it;
            }
          }

          message = message.substr(message_length);
        } while (message.size() != 0);
      }
    });
  }

private:
  std::string ip_;
  uint16_t port_;
  int32_t server_socket_;
  std::unordered_set<int32_t> client_sockets_;

  bool stop_{false};
  std::mutex messages_mutex_;
  std::condition_variable messages_condition_;
  std::thread server_thread_;

  std::mutex clients_mutex_;
  std::condition_variable clients_condition_;
  std::thread client_thread_;

  std::list<std::string> messages_;
};

#endif // CHANNEL_SERVER_H
