#ifndef CHANNEL_LOG_H
#define CHANNEL_LOG_H

#include <cstdint>
#include <cstdio>

class Log {
 public:
  inline static const char *kLevelName[]{"RAW", "ERROR", "WARN", "INFO", "DEBUG"};
  inline static int32_t kLevel = 1;

  static void set_level(int32_t level) { kLevel = level; }

  template <typename... Args> static void raw(const char *format, Args... args) {
    if (kLevel >= 0) {
      if constexpr (sizeof...(args) == 0) {
        printf("%s", format);
      } else {
        printf(format, args...);
      }
    }
  }

  template <typename... Args> static void error(const char *format, Args... args) {
    if (kLevel >= 1) {
      printf("[%s] ", kLevelName[1]);
      if constexpr (sizeof...(args) == 0) {
        printf("%s", format);
      } else {
        printf(format, args...);
      }
      printf("\n");
    }
  }

  template <typename... Args> static void warn(const char *format, Args... args) {
    if (kLevel >= 2) {
      printf("[%s] ", kLevelName[2]);
      if constexpr (sizeof...(args) == 0) {
        printf("%s", format);
      } else {
        printf(format, args...);
      }
      printf("\n");
    }
  }

  template <typename... Args> static void info(const char *format, Args... args) {
    if (kLevel >= 3) {
      printf("[%s] ", kLevelName[3]);
      if constexpr (sizeof...(args) == 0) {
        printf("%s", format);
      } else {
        printf(format, args...);
      }
      printf("\n");
    }
  }

  template <typename... Args> static void debug(const char *format, Args... args) {
    if (kLevel >= 4) {
      printf("[%s] ", kLevelName[4]);
      if constexpr (sizeof...(args) == 0) {
        printf("%s", format);
      } else {
        printf(format, args...);
      }
      printf("\n");
    }
  }
};

#endif  // CHANNEL_LOG_H
