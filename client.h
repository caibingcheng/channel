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
#include "utils.h"

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

    // clear socker buffer first
    char buffer[kMaxMessageSize];
    for (int64_t clear_bytes = read(client_socket_, buffer, sizeof(buffer)); clear_bytes > 0;) {
      Log::debug("Clear socket buffer %ld bytes", clear_bytes);
    }
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

    auto print_message = [](const Message *msg) {
      Log::error("Message Info:");
      Log::error("  Version: %u", msg->header.version);
      Log::error("  Size: %u", msg->header.size);
      Log::error("  Index: %lu", msg->body.index);
      Log::error("  Generate Timestamp: %ld", msg->body.generate_timestamp);
      Log::error("  Send Timestamp: %ld", msg->body.send_timestamp);
      Log::error("  Send Bytes: %lu", msg->body.send_bytes);
      Log::error("  Length: %lu", msg->body.length);
    };

    Hist<uint64_t> send_delay_us_hist("send delay", {50, 100, 500, 1000});
    Hist<uint64_t> generate_delay_us_hist("generate delay", {50, 100, 500, 1000, 10000});
    const int64_t message_size = static_cast<int64_t>(sizeof(Message));
    uint64_t prev_length = message_size + kMaxMessageSize + 1;
    std::unique_ptr<char[]> message = std::make_unique<char[]>(prev_length);
    struct epoll_event events[1];
    while (true) {
      if (epoll_wait(epoll_fd, events, 1, -1) < 0) {
        Log::error("Failed to wait for epoll");
        break;
      }

      int64_t read_bytes = read(client_socket_, message.get(), message_size);
      if (read_bytes <= 0) {
        Log::error("Connection closed");
        break;
      }
      if (read_bytes < message_size) {
        Log::error("Failed to read message");
        break;
      }
      const Message *msg = reinterpret_cast<Message *>(message.get());
      if (msg->header.version != kVersion.version || msg->header.size != message_size) {
        Log::error("Invalid message version or size, version: %u vs %u, size: %u vs %lu", msg->header.version,
                   kVersion.version, msg->header.size, message_size);
        print_message(msg);
        break;
      }

      Log::debug("Message Info:");
      Log::debug("  Version: %u", msg->header.version);
      Log::debug("  Size: %u", msg->header.size);
      Log::debug("  Index: %lu", msg->body.index);
      Log::debug("  Generate Timestamp: %ld", msg->body.generate_timestamp);
      Log::debug("  Send Timestamp: %ld", msg->body.send_timestamp);
      Log::debug("  Send Bytes: %lu", msg->body.send_bytes);
      Log::debug("  Length: %lu", msg->body.length);
      const uint64_t current_length = message_size + msg->body.length + 1;
      if (current_length > prev_length) {
        std::unique_ptr<char[]> new_message = std::make_unique<char[]>(current_length);
        if (new_message == nullptr) {
          Log::error("Failed to allocate message buffer");
          print_message(msg);
          break;
        }
        memcpy(new_message.get(), msg, message_size);
        message = std::move(new_message);
        msg = reinterpret_cast<Message *>(message.get());
        Log::debug("Reallocate message buffer from %lu to %lu", prev_length, current_length);
        prev_length = current_length;
      }
      read_bytes = read(client_socket_, message.get() + message_size, msg->body.length);
      if (read_bytes < static_cast<int64_t>(msg->body.length)) {
        Log::error("Failed to read message body, length: %lu", msg->body.length);
        print_message(msg);
        break;
      }
      const int64_t recv_timestamp = Message::timestamp_us();
      message[current_length - 1] = 0;

      if (msg->body.send_bytes <= recv_bytes_) {
        Log::error("Invalid send bytes, %lu vs %lu", msg->body.send_bytes, recv_bytes_);
        print_message(msg);
        break;
      }
      if (msg->body.generate_timestamp > recv_timestamp || msg->body.send_timestamp < 0) {
        Log::error("Invalid timestamp, generate: %lu, send: %lu, recv: %lu", msg->body.generate_timestamp,
                   msg->body.send_timestamp, recv_timestamp);
        print_message(msg);
        break;
      }

      Log::raw("%s", message.get() + message_size);
      recv_bytes_ += (msg->body.length + message_size);

      send_delay_us_hist.add(recv_timestamp - (msg->body.generate_timestamp + msg->body.send_timestamp));
      generate_delay_us_hist.add(recv_timestamp - msg->body.generate_timestamp);
      Log::debug("Received %lu bytes, Send %lu bytes, Index %lu", recv_bytes_, msg->body.send_bytes, msg->body.index);
      generate_delay_us_hist.print();
      send_delay_us_hist.print();
    }

    close(epoll_fd);
    close(client_socket_);
    client_socket_ = -1;
  }

 private:
  const char *ip_;
  uint16_t port_;
  int32_t client_socket_{-1};
  uint64_t recv_bytes_{0};
};

#endif  // CHANNEL_CLIENT_H