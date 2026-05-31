#ifndef TESTS_ETHERNET_MOCKSPI_H_
#define TESTS_ETHERNET_MOCKSPI_H_

#include <gmock/gmock.h>
#include <stdint.h>

// Mock für SPI-Interface
class MockSPI {
 public:
  virtual ~MockSPI() = default;

  MOCK_METHOD(uint8_t, transfer, (uint8_t data));
  MOCK_METHOD(void, transfer_buffer, (uint8_t * buf, uint16_t len));
};

// Mock für UART-Interface
class MockUART {
 public:
  virtual ~MockUART() = default;

  MOCK_METHOD(void, send_string, (const char* str));
  MOCK_METHOD(void, send_byte, (uint8_t byte));
};

#endif  // TESTS_ETHERNET_MOCKSPI_H_
