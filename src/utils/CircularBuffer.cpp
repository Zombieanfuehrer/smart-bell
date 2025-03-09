
#include "utils/CircularBuffer.h"

namespace utils
{
  bool CircularBuffer::push_back(const uint8_t data) {
    if (circ_buffer_full_()) {
      return false;
    }

    values_[tail_] = data;
    num_entries_++;
    tail_ = (tail_ +1) % max_entries_;

    return true;
  }

  bool CircularBuffer::pop_front(const uint8_t *data) {
    if (circ_buffer_empty_()) {
      return false;
    }

    auto res = values_[head_];
    head_ = (head_ + 1) % max_entries_;
    num_entries_--;

    return true;
  }

  size_t CircularBuffer::size() const {
    return num_entries_;
  }

  void CircularBuffer::clear() {

  }

  bool CircularBuffer::circ_buffer_empty_() const {
    return (num_entries_ == 0);
  }

  bool CircularBuffer::circ_buffer_full_() const {
    return (num_entries_ == max_entries_);
  }
} // namespace utils
