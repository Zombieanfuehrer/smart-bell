#include <avr/io.h>

#include "Serial/UART.h"

serial::Serial_parameters uart_parms = {
  serial::Communication_mode::kAsynchronous,
  serial::Asynchronous_mode::kNormal,
  serial::Baudrate::kBaud_9600,
  serial::StopBits::kOne,
  serial::DataBits::kEight,
  serial::Parity::kNone
  };

int main (void) {
  serial::UART uart(uart_parms);

  while (true)
  {
    if (uart.is_read_data_available()) {
      uint8_t data = uart.read_byte();
      uart.send(data);
      data = 0;
    }
    uart.send(static_cast<uint8_t>('F'));
  }
  
}