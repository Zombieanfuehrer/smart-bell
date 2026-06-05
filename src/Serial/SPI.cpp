#ifdef __AVR__
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#endif

#include <stdint.h>
#include <util/delay.h>

#include "Serial/SPI.h"

namespace serial {

// Diese Methode ist für beide Varianten identisch und steht außerhalb des Switches
void SPI::set_slave_select(const uint8_t slave_select) { slave_select_ = slave_select; }

}  // namespace serial

#ifdef USE_SPI_BUFFERED
// =========================================================================
// VARIANTE A: BUFFERED & INTERRUPT-DRIVEN (Deine ursprüngliche Logik)
// =========================================================================
#include "Utils/CircularBuffer.h"

volatile static uint8_t tx_rx_done = 0;
volatile uint8_t err_dummy_ = 0;
static utils::CircularBuffer<serial::SPI::kTXRX_buffer_size> rx_buffer_;
static utils::CircularBuffer<serial::SPI::kTXRX_buffer_size> tx_buffer_;

ISR(SPI_STC_vect) {
  if (SPSR & (1 << WCOL)) {
    // Write collision
    err_dummy_ = SPDR;
    err_dummy_ = 0;
    tx_rx_done = 1;
  } else {
    rx_buffer_.push_back(SPDR);
    tx_rx_done = 1;
  }
}

namespace serial {

SPI::SPI(const SPI_parameters &parameters, const uint8_t slave_select)
    : slave_select_(slave_select) {
  // Set MOSI and SCK output, all others input
  DDRB |= kSCK | kMOSI;

  // Enable SPI, Master, set clock rate, ENABLE SPI INTERRUPT (SPIE)
  SPCR = static_cast<uint8_t>(parameters.spi_mode) | static_cast<uint8_t>(parameters.data_order) |
         static_cast<uint8_t>(parameters.clock_polarity) |
         static_cast<uint8_t>(parameters.clock_phase) |
         static_cast<uint8_t>(parameters.clock_rate) | (1 << SPE) | (1 << SPIE);
}

void SPI::send(const uint8_t byte) {
  tx_buffer_.push_back(byte);
  this->send_();
}

void SPI::send_bytes(const uint8_t *const bytes, const uint16_t length) {
  for (uint16_t i = 0; i < length; i++) {
    tx_buffer_.push_back(bytes[i]);
  }
  this->send_();
}

void SPI::send_string(const char *string) {
  uint16_t nByte = 0;
  while (string[nByte] != '\0') {
    tx_buffer_.push_back(string[nByte]);
    nByte++;
  }
  this->send_();
}

bool SPI::is_read_data_available() const { return (rx_buffer_.used_entries() > 0); }

uint8_t SPI::read_byte() {
  uint8_t byte;
  if (rx_buffer_.pop_front(&byte)) {
    return byte;
  }
  return 0;  // Standardwert, falls der Puffer leer ist
}

void SPI::send_() {
  uint8_t byte;
  tx_rx_done = 0;
  PORTB &= ~(1 << slave_select_);
  _delay_us(100);
  while (tx_buffer_.pop_front(&byte)) {
    wdt_reset();
    SPDR = byte;
    while (!tx_rx_done)
      ;
  }
  PORTB |= (1 << slave_select_);
  _delay_us(100);
}

}  // namespace serial

#else
// =========================================================================
// VARIANTE B: DIRECT POLLING (0 Byte RAM-Verbrauch, ideal für W5500)
// =========================================================================

namespace serial {

SPI::SPI(const SPI_parameters &parameters, const uint8_t slave_select)
    : slave_select_(slave_select) {
  // Set MOSI and SCK output, all others input
  DDRB |= kSCK | kMOSI;

  // Enable SPI, Master, set clock rate, KEIN SPI INTERRUPT (SPIE bleibt 0)
  SPCR = static_cast<uint8_t>(parameters.spi_mode) | static_cast<uint8_t>(parameters.data_order) |
         static_cast<uint8_t>(parameters.clock_polarity) |
         static_cast<uint8_t>(parameters.clock_phase) |
         static_cast<uint8_t>(parameters.clock_rate) | (1 << SPE);
}

void SPI::send(const uint8_t byte) {
  PORTB &= ~(1 << slave_select_);
  _delay_us(100);

  SPDR = byte;
  while (!(SPSR & (1 << SPIF))) {
    // Synchrones Warten, bis das Byte vollständig übertragen wurde
  }

  PORTB |= (1 << slave_select_);
  _delay_us(100);
}

void SPI::send_bytes(const uint8_t *const bytes, const uint16_t length) {
  PORTB &= ~(1 << slave_select_);
  _delay_us(100);

  for (uint16_t i = 0; i < length; i++) {
    wdt_reset();  // Watchdog während potenziell langer Übertragungen zurücksetzen
    SPDR = bytes[i];
    while (!(SPSR & (1 << SPIF))) {
      // Warten auf Hardware-Flag
    }
  }

  PORTB |= (1 << slave_select_);
  _delay_us(100);
}

void SPI::send_string(const char *string) {
  if (!string)
    return;

  PORTB &= ~(1 << slave_select_);
  _delay_us(100);

  uint16_t nByte = 0;
  while (string[nByte] != '\0') {
    wdt_reset();
    SPDR = static_cast<uint8_t>(string[nByte]);
    while (!(SPSR & (1 << SPIF))) {
      // Warten auf Hardware-Flag
    }
    nByte++;
  }

  PORTB |= (1 << slave_select_);
  _delay_us(100);
}

bool SPI::is_read_data_available() const {
  // Im synchronen Polling-Modus blockiert die Übertragung, bis Daten da sind.
  // Wir geben true zurück, damit die App weiß, dass sie direkt lesen darf.
  return true;
}

uint8_t SPI::read_byte() {
  PORTB &= ~(1 << slave_select_);
  _delay_us(100);

  // Ein Dummy-Byte (0xFF) senden, um das Clock-Signal
  // der Master-Hardware für den Lese-Vorgang zu generieren
  SPDR = 0xFF;
  while (!(SPSR & (1 << SPIF))) {
    // Warten auf Hardware-Flag
  }

  uint8_t byte = SPDR;  // Empfangenes Byte aus dem Register sichern

  PORTB |= (1 << slave_select_);
  _delay_us(100);

  return byte;
}

}  // namespace serial

#endif  // USE_SPI_BUFFERED