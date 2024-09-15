#include <avr/io.h>
#include <avr/interrupt.h>

#include <cstdint>

#include "Serial/UART.h"
#include "UART.h"

ISR (USART_RX_vect) {

}

ISR (USART_TX_vect) {
  UART::Tx_busy_ = UART::is_not_busy;
}

namespace serial {

  UART::UART(const Serial_parameters &serial_parameters) {

    auto prescaler = calculate_baudrate_prescaler(static_cast<Baudrate>(serial_parameters.Baudrate), static_cast<Asynchronous_mode>(serial_parameters.Asynchronous_mode));
    // Set baud rate
    UBRR0H = prescaler >> 8;
    UBRR0L = prescaler;
    // Receiver and transmitter enable with interrupts
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0) | (1 << TXCIE0);
    // Set frame format and communication mode
    UCSR0C = serial_parameters.Communication_mode | serial_parameters.Stop_bits | serial_parameters.Data_bits | serial_parameters.Parity_mode;

  }

  void UART::send_byte(const uint8_t byte) {
    while (Tx_busy_);
    Tx_busy_ = UART::is_busy;
    UDR0 = byte;
  }

  void UART::send_bytes(const uint8_t const *bytes, const uint16_t lengths) {
    for (uint16_t i = 0; i < lengths; i++) {
      this->send_byte(bytes[i]);
    }
  }

  void UART::send_string(const char *string) {
    uint16_t nByte{0};
    do {
      this->send_byte(string[nByte]);
      nByte++;
    }while (string[nByte] != '\0');
    this->send_byte(string[nByte]);
    

  }

  UART::~UART() {}

  static uint8_t UART::calculate_baudrate_prescaler(const Baudrate &baudrate, const Asynchronous_mode &asynchronous_mode) {
    if (asynchronous_mode == Asynchronous_mode::kDouble_speed) {
      return (F_CPU / (kAsynchronous_double_speed_mode * static_cast<uint16_t>(baudrate)) ) - 1;
    }
    return (F_CPU / (kAsynchronous_normal_speed_mode * static_cast<uint16_t>(baudrate)) ) - 1;
  }

}  // namespace serial