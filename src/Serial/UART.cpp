#ifdef __AVR__
#include <avr/interrupt.h>
#include <avr/io.h>
#endif
#include <stdint.h>

#include "Serial/UART.h"

namespace {

volatile uint8_t tx_busy_ = serial::UART::is_not_busy;

volatile uint8_t rx_head_ = 0;
volatile uint8_t rx_tail_ = 0;
uint8_t rx_buffer_[serial::UART::kRX_buffer_size] = {0};

volatile uint8_t tx_head_ = 0;
volatile uint8_t tx_tail_ = 0;
uint8_t tx_buffer_[serial::UART::kTX_buffer_size] = {0};

static_assert((serial::UART::kRX_buffer_size & (serial::UART::kRX_buffer_size - 1)) == 0,
              "UART RX buffer size must be a power of two");
static_assert((serial::UART::kTX_buffer_size & (serial::UART::kTX_buffer_size - 1)) == 0,
              "UART TX buffer size must be a power of two");

inline uint8_t ring_next(uint8_t index, uint8_t mask) {
  return static_cast<uint8_t>((index + 1) & mask);
}

}  // namespace

ISR(USART_RX_vect) {
  uint8_t received_byte = UDR0;
  const uint8_t next_head =
      ring_next(rx_head_, static_cast<uint8_t>(serial::UART::kRX_buffer_size - 1));

  if (next_head != rx_tail_) {
    rx_buffer_[rx_head_] = received_byte;
    rx_head_ = next_head;
  }
}

ISR(USART_UDRE_vect) {
  if (tx_tail_ != tx_head_) {
    UDR0 = tx_buffer_[tx_tail_];
    tx_tail_ = ring_next(tx_tail_, static_cast<uint8_t>(serial::UART::kTX_buffer_size - 1));
  } else {
    // Buffer empty: MUST disable UDRIE or ISR fires endlessly
    UCSR0B &= ~(1 << UDRIE0);
    tx_busy_ = serial::UART::is_not_busy;
  }
}

namespace serial {

UART::UART(const Serial_parameters &serial_parameters) {
  bool were_interrupts_enabled = (SREG & (1 << SREG_I));
  if (were_interrupts_enabled) {
    cli();
  }

  if (serial_parameters.asynchronous_mode == Asynchronous_mode::kDouble_speed) {
    UCSR0A |= (1 << U2X0);  // Enable double speed mode
  } else {
    UCSR0A &= ~(1 << U2X0);  // Disable double speed mode
  }

  auto prescaler =
      calculate_baudrate_prescaler(serial_parameters.baudrate, serial_parameters.asynchronous_mode);
  // Set baud rate
  UBRR0L = static_cast<uint8_t>(prescaler & 0xFF);
  UBRR0H = static_cast<uint8_t>(prescaler >> 8);

  // Receiver and transmitter enable with interrupts
  UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
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
  // Prevent silent character loss when TX buffer is temporarily full.
  const uint8_t mask = static_cast<uint8_t>(kTX_buffer_size - 1);
  uint8_t next_head = ring_next(tx_head_, mask);
  while (next_head == tx_tail_) {
  }
  tx_buffer_[tx_head_] = byte;
  tx_head_ = next_head;
  if (!(UCSR0B & (1 << UDRIE0))) {
    tx_busy_ = serial::UART::is_not_busy;
  }
  send_();
}

void UART::send_bytes(const uint8_t *const bytes, const uint16_t lengths) {
  const uint8_t mask = static_cast<uint8_t>(kTX_buffer_size - 1);
  for (uint16_t i = 0; i < lengths; i++) {
    uint8_t next_head = ring_next(tx_head_, mask);
    while (next_head == tx_tail_) {
    }
    tx_buffer_[tx_head_] = bytes[i];
    tx_head_ = next_head;
  }
  if (!(UCSR0B & (1 << UDRIE0))) {
    tx_busy_ = serial::UART::is_not_busy;
  }
  send_();
}

void UART::send_string(const char *string) {
  uint16_t nByte = 0;
  const uint8_t mask = static_cast<uint8_t>(kTX_buffer_size - 1);
  while (string[nByte] != '\0') {
    uint8_t next_head = ring_next(tx_head_, mask);
    while (next_head == tx_tail_) {
    }
    tx_buffer_[tx_head_] = static_cast<uint8_t>(string[nByte]);
    tx_head_ = next_head;
    nByte++;
  }
  if (!(UCSR0B & (1 << UDRIE0))) {
    tx_busy_ = serial::UART::is_not_busy;
  }
  send_();
}

bool UART::is_read_data_available() const { return (rx_tail_ != rx_head_); }

uint8_t UART::read_byte() {
  if (rx_tail_ != rx_head_) {
    uint8_t byte = rx_buffer_[rx_tail_];
    rx_tail_ = ring_next(rx_tail_, static_cast<uint8_t>(kRX_buffer_size - 1));
    return byte;
  }
  // Handle the case where no data is available
  return 0;  // or any other default value or error code
}

uint8_t UART::calculate_baudrate_prescaler(const Baudrate &baudrate,
                                           const Asynchronous_mode &asynchronous_mode) {
  if (asynchronous_mode == Asynchronous_mode::kDouble_speed) {
    return static_cast<uint8_t>(
        (F_CPU / (kAsynchronous_double_speed_mode * static_cast<uint32_t>(baudrate))) - 1);
  }
  return static_cast<uint8_t>(
      (F_CPU / (kAsynchronous_normal_speed_mode * static_cast<uint32_t>(baudrate))) - 1);
}

void UART::send_() {
  if (tx_busy_ == is_not_busy && tx_tail_ != tx_head_) {
    tx_busy_ = serial::UART::is_busy;
    UDR0 = tx_buffer_[tx_tail_];
    tx_tail_ = ring_next(tx_tail_, static_cast<uint8_t>(kTX_buffer_size - 1));
    UCSR0B |= (1 << UDRIE0);  // Enable the interrupt
  }
}

}  // namespace serial