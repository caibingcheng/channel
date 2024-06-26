#ifndef CHANNEL_UTILS_H
#define CHANNEL_UTILS_H

#include "config.h"
#include "log.h"

#include <algorithm>
#include <vector>

inline const union {
  uint32_t version;
  struct {
    uint8_t patch;
    uint8_t minor;
    uint8_t major;
  } detail;
} kVersion{.detail = {
               .patch = 1,
               .minor = 1,
               .major = 0,
           }};

struct Message {
  struct {
    uint32_t version : 24;
    uint32_t size : 8;
  } header = {
      .version = kVersion.version,
      .size = sizeof(Message),
  };

  struct __attribute__((packed)) {
    int64_t generate_timestamp{0};
    int32_t send_timestamp{0};
    uint32_t index{0};
    uint64_t send_bytes : 48;
    // data length excluding Message
    uint64_t length : 16;
  } body;

  static int64_t timestamp_us() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
  }
};

template <typename Tp> class Hist {
 public:
  Hist(const std::string &name, const std::initializer_list<Tp> &hist_delimiters)
      : name_(name), hist_delimiters_(hist_delimiters) {
    hist_.resize(hist_delimiters.size() + 1);
  }

  void add(const Tp &value) {
    auto it = std::lower_bound(hist_delimiters_.begin(), hist_delimiters_.end(), value);
    ++hist_[it - hist_delimiters_.begin()];

    mean_ = (mean_ * count_ + value) / (count_ + 1);
    count_ += 1;
    min_ = std::min(min_, value);
    max_ = std::max(max_, value);
  }

  std::vector<uint64_t> get() const { return hist_; }
  Tp min() const { return min_; }
  Tp max() const { return max_; }
  Tp mean() const { return mean_; }
  void print() const {
    Log::debug("Hist of %s (min:%lu max:%lu mean:%lu count:%lu), histogram:", name_.c_str(), min_, max_, mean_, count_);
    for (size_t i = 0; i < hist_.size(); ++i) {
      Log::debug("  %s: %lu(%.2lf%)",
                 (i >= hist_delimiters_.size()) ? "overflow" : std::to_string(hist_delimiters_[i]).c_str(), hist_[i],
                 100.0 * static_cast<double>(hist_[i]) / static_cast<double>(count_));
    }
  }

 private:
  std::string name_{""};
  std::vector<Tp> hist_delimiters_;
  std::vector<uint64_t> hist_;
  Tp min_{std::numeric_limits<Tp>::max()};
  Tp max_{std::numeric_limits<Tp>::min()};
  Tp mean_{0};
  uint64_t count_{0};
};

#endif  // CHANNEL_UTILS_H
