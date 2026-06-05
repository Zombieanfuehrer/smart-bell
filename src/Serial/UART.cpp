#ifdef __AVR__
#include <avr/interrupt.h>
#include <avr/io.h>
#endif
#include <stdint.h>

#include "Serial/UART.h"

namespace {

volatile uint8_t rx_head_ = 0;
volatile uint8_t rx_tail_ = 0;
uint8_t rx_buffer_[serial::UART::kRX_buffer_size] = {0};

static_assert((serial::UART::kRX_buffer_size & (serial::UART::kRX_buffer_size - 1)) == 0,
              "UART RX buffer size must be a power of two");

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
  // TX is handled in polling mode for deterministic behavior.
  UCSR0B &= ~(1 << UDRIE0);
}

namespace serial {

UART::UART(const Serial_parameters &serial_parameters) {
  cli();

  // 1. Register komplett nullen, um alten Bootloader-Müll loszuwerden
  UCSR0A = 0;
  UCSR0B = 0;
  UCSR0C = 0;

  // 2. Double Speed Modus konfigurieren
  if (serial_parameters.asynchronous_mode == Asynchronous_mode::kDouble_speed) {
    UCSR0A |= (1 << U2X0);
  }

  // REPARIERT: Nutzt nun uint16_t um Abschneidefehler über 255 zu verhindern!
  uint16_t prescaler =
      calculate_baudrate_prescaler(serial_parameters.baudrate, serial_parameters.asynchronous_mode);

  // 16-Bit Baudratenregister sicher beschreiben
  UBRR0H = static_cast<uint8_t>(prescaler >> 8);
  UBRR0L = static_cast<uint8_t>(prescaler & 0xFF);

  // 3. Empfänger und Sender aktivieren + RX-Interrupt einschalten
  UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);

  // REPARIERT: Erzwingt ein sauberes 8N1 Frame-Format (8 Datenbits, keine Parität, 1 Stop-Bit)
  // Das umgeht fehlerhafte Bit-Verschiebungen aus den abstrakten Enum-Klassen.
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

  sei();
}

void UART::send(const uint8_t byte) {
  while (!(UCSR0A & (1 << UDRE0))) {
  }
  UDR0 = byte;
}

void UART::send_bytes(const uint8_t *const bytes, const uint16_t lengths) {
  for (uint16_t i = 0; i < lengths; i++) {
    while (!(UCSR0A & (1 << UDRE0))) {
    }
    UDR0 = bytes[i];
  }
}

void UART::send_string(const char *string) {
  uint16_t nByte = 0;
  while (string[nByte] != '\0') {
    while (!(UCSR0A & (1 << UDRE0))) {
    }
    UDR0 = static_cast<uint8_t>(string[nByte]);
    nByte++;
  }
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

uint16_t UART::calculate_baudrate_prescaler(const Baudrate &baudrate,
                                            const Asynchronous_mode &asynchronous_mode) {
  uint32_t target_baud = static_cast<uint32_t>(baudrate);

  if (asynchronous_mode == Asynchronous_mode::kDouble_speed) {
    return static_cast<uint16_t>((F_CPU / (8UL * target_baud)) - 1);
  }
  return static_cast<uint16_t>((F_CPU / (16UL * target_baud)) - 1);
}

void UART::send_() {
  // Polling TX path does not need kickoff logic.
}

}  // namespace serial