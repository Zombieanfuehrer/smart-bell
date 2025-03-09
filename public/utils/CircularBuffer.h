#ifndef PUBLIC_UTILS_CIRCULARBUFFER_H_
#define PUBLIC_UTILS_CIRCULARBUFFER_H_

#include <avr/io.h>
#include <stdint.h>
#include <stddef.h>

namespace utils
{

class CircularBuffer
{

 public:
  CircularBuffer(const size_t max_buffer_size = 250);
  ~CircularBuffer() = default;
  bool push_back(const uint8_t data);
  bool pop_front(const uint8_t *data);
  size_t used_entries() const;
  void clear();

 private:
  uint8_t * const values_ {nullptr};
  int head_;
  int tail_;
  size_t num_entries_;
  const size_t max_entries_;

 private:
  bool circ_buffer_empty_() const;
  bool circ_buffer_full_() const;

};


    
} // namespace utils



#endif  // PUBLIC_UTILS_CIRCULARBUFFER_H_