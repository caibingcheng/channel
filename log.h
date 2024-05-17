#ifndef CHANNEL_LOG_H
#define CHANNEL_LOG_H

#include <cstdint>
#include <cstdio>

#include <algorithm>
#include <unordered_map>

class Log {
 private:
  inline static int32_t kLevel_ = 1;

  template <typename... Args> inline static void print_(const char *format, Args &&... args) {
    if constexpr (sizeof...(args) == 0) {
      printf("%s", format);
    } else {
      printf(format, args...);
    }
  }

 public:
  static void set_level(int32_t level) { kLevel_ = level; }
  static int32_t get_level() { return kLevel_; }

  template <typename... Args> static void raw(const char *format, Args &&... args) {
    print_(format, std::forward<Args>(args)...);
  }

  template <typename... Args> static void error(const char *format, Args &&... args) {
    constexpr const char name[] = "ERROR";
    constexpr const int32_t level = 1;
    if (get_level() >= level) {
      printf("[%s] ", name);
      print_(format, args...);
      printf("\n");
    }
  }

  template <typename... Args> static void warn(const char *format, Args &&... args) {
    constexpr const char name[] = "WARN";
    constexpr const int32_t level = 2;
    if (get_level() >= level) {
      printf("[%s] ", name);
      print_(format, args...);
      printf("\n");
    }
  }

  template <typename... Args> static void info(const char *format, Args &&... args) {
    constexpr const char name[] = "INFO";
    constexpr const int32_t level = 3;
    if (get_level() >= level) {
      printf("[%s] ", name);
      print_(format, args...);
      printf("\n");
    }
  }

  template <typename... Args> static void debug(const char *format, Args &&... args) {
    constexpr const char name[] = "DEBUG";
    constexpr const int32_t level = 4;
    if (get_level() >= level) {
      printf("[%s] ", name);
      print_(format, args...);
      printf("\n");
    }
  }
};

#endif  // CHANNEL_LOG_H
