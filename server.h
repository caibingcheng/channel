#ifndef CHANNEL_SERVER_H
#define CHANNEL_SERVER_H

#include <arpa/inet.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <condition_variable>
#include <fstream>
#include <iostream>
#include <list>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>

#include "config.h"
#include "log.h"
#include "utils.h"

class Server {
 public:
  Server(uint16_t port) {
    port_ = port;
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    close(server_socket_);

    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) {
      throw "Failed to create server socket\n";
    }

    int32_t opt = 1;
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_);
    server_address.sin_addr.s_addr = inet_addr("0.0.0.0");

    if (bind(server_socket_, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
      throw "Failed to bind server socket\n";
    }

    if (listen(server_socket_, kMaxClientConnections) < 0) {
      throw "Failed to listen on server socket\n";
    }

    process_();
  }

  ~Server() {
    stop_ = true;
    messages_condition_.notify_all();
    clients_condition_.notify_all();

    if (client_thread_.joinable()) {
      client_thread_.join();
    }
    {
      std::lock_guard<std::mutex> lock(clients_mutex_);
      for (int32_t client_socket : client_sockets_) {
        close(client_socket);
      }
    }
    Log::debug("Client stopped");

    if (server_thread_.joinable()) {
      server_thread_.join();
    }
    close(server_socket_);
    Log::debug("Server stopped");
  }

  void send_message(bool is_drop) {
    std::string message = "";
    auto &bstream = std::cin;
    while (std::getline(bstream, message) && !stop_) {
      if (!message.empty()) {
        message += "\n";
        Log::debug("Message %lu Bytes", message.size());
        Log::raw("%s", message.c_str());
        Message msg;
        msg.body.length = message.size();
        msg.body.generate_timestamp = Message::timestamp_ns();
        message = std::string(reinterpret_cast<char *>(&msg), sizeof(Message)) + message;
        send_message_(message, is_drop);
      }
    }

    Log::debug("Waiting for sender to stop");
    {
      std::unique_lock<std::mutex> lock(messages_mutex_);
      messages_condition_.wait(lock, [this] { return messages_.empty() || stop_; });
    }
    Log::debug("Sender stopped");
  }

 private:
  void send_message_(const std::string &message, bool is_drop = true) {
    {
      std::lock_guard<std::mutex> lock(messages_mutex_);
      messages_.push_back(message);
      if (is_drop && (messages_.size() > kMaxMessageQueueSize)) {
        Log::debug("Drop message: %s", messages_.front().c_str());
        messages_.pop_front();
      }
    }
    messages_condition_.notify_one();
  }

  void process_() {
    client_thread_ = std::thread([this]() {
      const int32_t epoll_fd = epoll_create1(0);
      if (epoll_fd == -1) {
        throw "Failed to create epoll\n";
      }

      struct epoll_event event;
      event.events = EPOLLIN;
      event.data.fd = server_socket_;
      if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket_, &event) == -1) {
        throw "Failed to add server socket to epoll\n";
      }

      struct epoll_event events[1];
      while (!stop_) {
        const int32_t nfds = epoll_wait(epoll_fd, events, 1, 500);
        if (nfds < 0) {
          Log::error("Failed to wait epoll\n");
        }
        if (nfds <= 0) {
          continue;
        }

        const int32_t client_socket = accept(server_socket_, NULL, NULL);
        if (client_socket < 0) {
          Log::error("Failed to accept client\n");
          continue;
        }

        {
          std::lock_guard<std::mutex> lock(clients_mutex_);
          client_sockets_.insert(client_socket);
        }
        clients_condition_.notify_one();
        Log::debug("New client connected, socket: %d, client count: %lu", client_socket, client_sockets_.size());
      }

      close(epoll_fd);
    });

    server_thread_ = std::thread([this]() {
      std::string message = "";
      uint64_t send_bytes = 0;
      while (!stop_) {
        {
          std::unique_lock<std::mutex> lock(clients_mutex_);
          clients_condition_.wait(lock, [this] { return !client_sockets_.empty() || stop_; });
          if (stop_) {
            break;
          }
        }

        {
          std::unique_lock<std::mutex> lock(messages_mutex_);
          messages_condition_.wait(lock, [this] { return !messages_.empty() || stop_; });
          if (stop_) {
            break;
          }
        }

        {
          message = "";
          std::lock_guard<std::mutex> lock(messages_mutex_);
          while (message.size() < kMaxMessageSize && !messages_.empty()) {
            const std::string &front_message = messages_.front();
            if (!message.empty()) {
              Message *const origin_msg = reinterpret_cast<Message *>(const_cast<char *>(message.c_str()));
              const Message *const current_msg = reinterpret_cast<Message *>(const_cast<char *>(front_message.c_str()));
              origin_msg->body.length += current_msg->body.length;
              message += front_message.substr(sizeof(Message));
            } else {
              message += messages_.front();
            }
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

        Message *msg = reinterpret_cast<Message *>(const_cast<char *>(message.c_str()));
        send_bytes += message.size();
        msg->body.index = index_++;
        msg->body.send_timestamp = Message::timestamp_ns();
        msg->body.send_bytes = send_bytes;
        for (auto it = client_sockets.begin(); it != client_sockets.end();) {
          const int32_t client_socket = *it;
          const ssize_t length = message.size();
          const bool is_alive = (send(client_socket, message.c_str(), length, MSG_NOSIGNAL) == length);
          if (!is_alive) {
            close(client_socket);
            {
              std::lock_guard<std::mutex> lock(clients_mutex_);
              client_sockets_.erase(client_socket);
            }
            it = client_sockets.erase(it);
            Log::debug("Client disconnected, socket: %d, client count: %lu", client_socket, client_sockets.size());
          } else {
            ++it;
          }
          Log::debug("Send %lu Bytes, curernt %ld", send_bytes, length);
        }

        // notify sender
        messages_condition_.notify_one();
      }
    });
  }

 private:
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

  uint64_t index_{0};
};

#endif  // CHANNEL_SERVER_H
