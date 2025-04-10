
#include "Utils/CircularBuffer.h"

namespace utils
{
  CircularBuffer::CircularBuffer(const size_t max_buffer_size)
    : max_entries_(max_buffer_size),
     head_(0), 
     tail_(0), 
     num_entries_(0)
  {}

  bool CircularBuffer::push_back(const uint8_t data) {
    if (circ_buffer_full_()) {
      return false;
    }

    values_[tail_] = data;
    num_entries_++;
    tail_ = (tail_ +1) % max_entries_;

    return true;
  }

  bool CircularBuffer::pop_front(uint8_t * const data) {
    if (data == nullptr || circ_buffer_empty_()) {
      return false;
    }

    *data = values_[head_];
    head_ = (head_ + 1) % max_entries_;
    num_entries_--;

    return true;
  }

  size_t CircularBuffer::used_entries() const {
    return num_entries_;
  }

  void CircularBuffer::clear() {
    head_ = 0;
    tail_ = 0;
    num_entries_ = 0;
  }

  bool CircularBuffer::circ_buffer_empty_() const {
    return (num_entries_ == 0);
  }

  bool CircularBuffer::circ_buffer_full_() const {
    return (num_entries_ == max_entries_);
  }
} // namespace utils
