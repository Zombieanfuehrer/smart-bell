#ifdef __AVR__
#include <avr/io.h>
#include <avr/interrupt.h>
#endif
#include <stdint.h>

#include "Serial/UART.h"
#include "Utils/CircularBuffer.h"

volatile static uint8_t tx_busy_ = serial::UART::is_not_busy;
static utils::CircularBuffer rx_buffer_ {serial::UART::kRX_buffer_size};
static utils::CircularBuffer tx_buffer_ {serial::UART::kTX_buffer_size};

ISR (USART_RX_vect) {
  rx_buffer_.push_back(UDR0);
}

ISR (USART_TX_vect) {
  tx_busy_ = serial::UART::is_not_busy;
}

namespace serial {
  
  UART::UART(const Serial_parameters &serial_parameters) {
    bool were_interrupts_enabled = (SREG & (1 << SREG_I));
    if (were_interrupts_enabled) {
      cli();
    }

    auto prescaler = calculate_baudrate_prescaler(serial_parameters.baudrate, serial_parameters.asynchronous_mode);
    // Set baud rate
    UBRR0L = static_cast<uint8_t>(prescaler & 0xFF);
    UBRR0H = static_cast<uint8_t>(prescaler >> 8);
    
    // Receiver and transmitter enable with interrupts
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0) | (1 << TXCIE0);
    // Set frame format and communication mode
    UCSR0C = static_cast<uint8_t>(serial_parameters.communication_mode) | 
      static_cast<uint8_t>(serial_parameters.stop_bits) |
      static_cast<uint8_t>(serial_parameters.data_bits) |
      static_cast<uint8_t>(serial_parameters.parity_mode);

    if (were_interrupts_enabled) {
      sei();
    }
  }

  void UART::send(const uint8_t byte) {
    tx_buffer_.push_back(byte);
    send_();
  }

  void UART::send_bytes(const uint8_t *const bytes, const uint16_t lengths) {
    for (uint16_t i = 0; i < lengths; i++) {
      tx_buffer_.push_back(bytes[i]);
    }
    send_();
  }

  void UART::send_string(const char *string) {
    uint16_t nByte = 0;
    while (string[nByte] != '\0') {
      tx_buffer_.push_back(string[nByte]);
      nByte++;
    }
    send_();
  }

  bool UART::is_read_data_available() const { 
    return (rx_buffer_.used_entries() > 0);
  }

  uint8_t UART::read_byte() {
    uint8_t byte;
    if (rx_buffer_.pop_front(&byte)) {
      return byte;
    } else {
      // Handle the case where no data is available
      return 0; // or any other default value or error code
    }
  }

  uint8_t UART::calculate_baudrate_prescaler(const Baudrate &baudrate, const Asynchronous_mode &asynchronous_mode) {
    if (asynchronous_mode == Asynchronous_mode::kDouble_speed) {
      return static_cast<uint8_t>((F_CPU / (kAsynchronous_double_speed_mode * static_cast<uint32_t>(baudrate)) ) - 1);
    }
    return static_cast<uint8_t>((F_CPU / (kAsynchronous_normal_speed_mode * static_cast<uint32_t>(baudrate)) ) - 1);
  }

  void UART::send_() {
    uint8_t byte;
    while (tx_buffer_.pop_front(&byte)) {
      while (tx_busy_) {
        ; // Wait for the transmission to finish
      }
      cli(); // Disable interrupts
      tx_busy_ = UART::is_busy;
      sei(); // Re-enable interrupts
      UDR0 = byte;
    }
  }

}  // namespace serial