/**
 *  @author Jakob Otto
 *  @email jakob.otto@haw-hamburg.de
 *  @date 09.04.2021
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <sstream>

namespace benchmark {

struct result {
  result()
    : received_bytes_(0),
      sent_bytes_(0),
      num_read_events_(0),
      num_write_events_(0),
      num_closed_(0) {
    // nop
  }

  void clear() {
    received_bytes_ = 0;
    sent_bytes_ = 0;
    num_read_events_ = 0;
    num_write_events_ = 0;
    num_closed_ = 0;
  }

  friend std::ostream& operator<<(std::ostream& os, result& res) {
    auto str = res.to_string();
    res.clear();
    return os << str;
  }

  void add_received_bytes(size_t received) {
    try_wait();
    received_bytes_ += received;
  }

  void add_sent_bytes(size_t sent) {
    try_wait();
    sent_bytes_ += sent;
  }

  void count_read_event() {
    try_wait();
    ++num_read_events_;
  }

  void count_write_event() {
    try_wait();
    ++num_write_events_;
  }

  void count_closed() {
    try_wait();
    ++num_closed_;
  }

  std::string to_string() {
    printing_ = true;
    std::stringstream ss;
    ss << received_bytes_ << ", " << sent_bytes_ << ", " << num_read_events_
       << ", " << num_write_events_ << ", " << num_closed_;
    clear();
    printing_ = false;
    cv.notify_all();
    return ss.str();
  }

private:
  void try_wait() {
    std::unique_lock<std::mutex> lk(access_lock_);
    cv.wait(lk, [&] { return !printing_; });
  }

  bool printing_;
  std::mutex access_lock_;
  std::condition_variable cv;
  size_t received_bytes_;
  size_t sent_bytes_;
  size_t num_read_events_;
  size_t num_write_events_;
  size_t num_closed_;
};

} // namespace benchmark
