#ifndef PUBLIC_UTILS_CIRCULARBUFFER_H_
#define PUBLIC_UTILS_CIRCULARBUFFER_H_

#include <stddef.h>
#include <stdint.h>

// Platform-specific includes
#ifdef __AVR__
#include <avr/io.h>
// On AVR, use volatile for ISR-safe operation
#define MAYBE_VOLATILE volatile
#else
// On Linux/tests, no need for volatile
#define MAYBE_VOLATILE
#endif

namespace utils {

// Template-based circular buffer with compile-time size
template <size_t N>
class CircularBuffer {
 public:
  CircularBuffer() : head_(0), tail_(0), num_entries_(0) {}
  ~CircularBuffer() = default;

  bool push_back(const uint8_t data) {
    if (circ_buffer_full_()) {
      return false;
    }
    values_[tail_] = data;
    num_entries_++;
    tail_ = (tail_ + 1) % N;
    return true;
  }

  bool pop_front(uint8_t* const data) {
    if (data == nullptr || circ_buffer_empty_()) {
      return false;
    }
    *data = values_[head_];
    head_ = (head_ + 1) % N;
    num_entries_--;
    return true;
  }

  size_t used_entries() const { return num_entries_; }

  void clear() {
    head_ = 0;
    tail_ = 0;
    num_entries_ = 0;
  }

 private:
  uint8_t values_[N] = {0};
  MAYBE_VOLATILE int head_;
  MAYBE_VOLATILE int tail_;
  MAYBE_VOLATILE size_t num_entries_;

  bool circ_buffer_empty_() const { return (num_entries_ == 0); }

  bool circ_buffer_full_() const { return (num_entries_ == N); }
};

}  // namespace utils

#endif  // PUBLIC_UTILS_CIRCULARBUFFER_H_