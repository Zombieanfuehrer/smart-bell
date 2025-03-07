#ifndef PUBLIC_SERIAL_INTERFACE_H_
#define PUBLIC_SERIAL_INTERFACE_H_

#include <stdint.h>

namespace serial {

class Interface
{
 public:
  Interface() = default;
  ~Interface() = default;
  virtual void send(const uint8_t byte) = 0;
  virtual void send_bytes(const uint8_t *const bytes, const uint16_t length) = 0;
  virtual void send_string(const char *string) = 0;
  virtual uint16_t is_read_data_available() const = 0;
  virtual uint8_t read_byte() = 0;
};


}  // serial



#endif