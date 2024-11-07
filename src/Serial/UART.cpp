#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <stdint.h>

#include "Serial/UART.h"

volatile static uint8_t Tx_busy_;
volatile static uint8_t Rx_buffer_[serial::UART::kRX_buffer_size] = {0};
volatile static uint16_t Rx_buffer_head_ = 0;
volatile static uint16_t Rx_buffer_tail_ = 0;

ISR (USART_RX_vect) {
  Rx_buffer_[Rx_buffer_tail_] = UDR0;
  Rx_buffer_tail_++;
  if (Rx_buffer_tail_ >= serial::UART::kRX_buffer_size) {
    Rx_buffer_tail_ = 0;
  }
}

ISR (USART_TX_vect) {
  Tx_busy_ = serial::UART::is_not_busy;
}

namespace serial {

  UART::UART(const Serial_parameters &serial_parameters) {
    cli();
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
    sei();
  }

  void UART::send_byte(const uint8_t byte) {
    while (Tx_busy_);
      wdt_reset();
    Tx_busy_ = UART::is_busy;
    UDR0 = byte;
  }

  void UART::send_bytes(const uint8_t *const bytes, const uint16_t lengths) {
    for (uint16_t i = 0; i < lengths; i++) {
      this->send_byte(bytes[i]);
    }
  }

  void UART::send_string(const char *string) {
    uint16_t nByte = 0;
    while (string[nByte] != '\0') {
        this->send_byte(string[nByte]);
        nByte++;
    }
  }

  uint16_t UART::is_read_data_available() const { 
    return (Rx_buffer_head_ != Rx_buffer_tail_);
  }

  uint8_t UART::read_byte() { 
    if (Rx_buffer_head_ == Rx_buffer_tail_) {
      return 0; // Buffer is empty
    }
    auto byte = Rx_buffer_[Rx_buffer_head_];
    Rx_buffer_head_++;
    if (Rx_buffer_head_ >= UART::kRX_buffer_size) {
      Rx_buffer_head_ = 0;
    }
    return byte;
  }

  uint8_t UART::calculate_baudrate_prescaler(const Baudrate &baudrate, const Asynchronous_mode &asynchronous_mode) {
    if (asynchronous_mode == Asynchronous_mode::kDouble_speed) {
      return static_cast<uint8_t>((F_CPU / (kAsynchronous_double_speed_mode * static_cast<uint32_t>(baudrate)) ) - 1);
    }
    return static_cast<uint8_t>((F_CPU / (kAsynchronous_normal_speed_mode * static_cast<uint32_t>(baudrate)) ) - 1);
  }

}  // namespace serial