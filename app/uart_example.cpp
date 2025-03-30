#ifdef __AVR__
#include <avr/io.h>
#include <util/delay.h>
#endif

#include "Serial/UART.h"

serial::Serial_parameters uart_parms = {
  serial::Communication_mode::kAsynchronous,
  serial::Asynchronous_mode::kNormal,
  serial::Baudrate::kBaud_9600,
  serial::StopBits::kOne,
  serial::DataBits::kEight,
  serial::Parity::kNone
  };

const char* motd = "uart example\n\r";
const char* debug_msg_ = "__data is available\n\r";
const uint8_t* debug_msg = reinterpret_cast<const uint8_t*>("data is available\n\r");

int main (void) {
  serial::UART uart(uart_parms);

  while (true)
  {
    if (uart.is_read_data_available()) {
      uart.send_string(debug_msg_);    
      char data = uart.read_byte();
      uart.send(data);
      data = 0;
    } else {
      uart.send_string(motd);
      _delay_ms(1500);
    }
  }
  
}