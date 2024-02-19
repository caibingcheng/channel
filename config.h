#ifndef CHANNEL_CONFIG_H
#define CHANNEL_CONFIG_H

#include <cstdint>

const char *kDefaultIP = "127.0.0.1";
const uint16_t kDefaultPort = 31719;
const uint32_t kMaxClientConnections = 1;
const uint32_t kMaxMessageSize = 4 * 1024;
const uint32_t kMaxMessageQueueSize = 1024;

#endif // CHANNEL_CONFIG_H