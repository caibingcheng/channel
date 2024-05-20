#ifndef CHANNEL_CONFIG_H
#define CHANNEL_CONFIG_H

#include <cstdint>

inline const char *kDefaultIP = "127.0.0.1";
inline const uint16_t kDefaultPort = 12121;
inline const uint32_t kMaxClientConnections = 1024;
inline const uint32_t kMaxMessageSize = 4096;
inline const uint32_t kMaxMessageQueueSize = 8 * 1024 * 1024 / kMaxMessageSize;

#endif  // CHANNEL_CONFIG_H