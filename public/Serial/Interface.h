#ifndef PUBLIC_SERIAL_INTERFACE_H_
#define PUBLIC_SERIAL_INTERFACE_H_

#include <stdint.h>

namespace serial {

class Interface
{
 public:
  Interface() = default;
  ~Interface() = default;
  virtual void send(const uint8_t byte) {}
  virtual void send_bytes(const uint8_t *const bytes, const uint16_t length) {}
  virtual void send_string(const char *string) {}
  virtual bool is_read_data_available() const { return 0; }
  virtual uint8_t read_byte() { return 0; }
};


}  // serial



#endif