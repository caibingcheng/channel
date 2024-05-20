#ifndef CHANNEL_CONFIG_H
#define CHANNEL_CONFIG_H

#include <cstdint>

inline const char *kDefaultIP = "127.0.0.1";
inline const uint16_t kDefaultPort = 12121;
inline const uint32_t kMaxClientConnections = 1024;
inline const uint32_t kMaxMessageSize = 4 * 1024 - 1;
inline const uint32_t kMaxMessageQueueSize = 1024;
inline const union {
  uint32_t version;
  struct {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
    uint8_t reserved;
  } detail;
} kVersion{.detail = {.major = 0, .minor = 1, .patch = 0, .reserved = 0}};


#endif  // CHANNEL_CONFIG_H