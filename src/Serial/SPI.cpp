#ifdef __AVR__
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#endif

#include <stdint.h>
#include <util/delay.h>

#include "Serial/SPI.h"
#include "Utils/CircularBuffer.h"

volatile static uint8_t tx_rx_done = 0;
volatile uint8_t err_dummy_ = 0;
static utils::CircularBuffer rx_buffer_ {serial::SPI::kTXRX_buffer_size};
static utils::CircularBuffer tx_buffer_ {serial::SPI::kTXRX_buffer_size};

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

  // Enable SPI, Master, set clock rate fck/16
  SPCR = static_cast<uint8_t>(parameters.spi_mode) |
         static_cast<uint8_t>(parameters.data_order) |
         static_cast<uint8_t>(parameters.clock_polarity) |
         static_cast<uint8_t>(parameters.clock_phase) |
         static_cast<uint8_t>(parameters.clock_rate) |
         (1 << SPE) | (1 << SPIE);
}

void SPI::set_slave_select(const uint8_t slave_select) {
  slave_select_ = slave_select;
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

bool SPI::is_read_data_available() const { 
  return (rx_buffer_.used_entries() > 0);
}

uint8_t SPI::read_byte() { 
  uint8_t byte;
  if (rx_buffer_.pop_front(&byte)) {
    return byte;
  }
  return 0; // Return a default value if the buffer is empty
}

void SPI::send_() {
  uint8_t byte;
  tx_rx_done = 0;
  PORTB &= ~(1 << slave_select_);
  _delay_us(100);
  while (tx_buffer_.pop_front(&byte))
  {        
    wdt_reset();   
    SPDR = byte;
    while (!tx_rx_done)
      ;
  }
    PORTB |= (1 << slave_select_);
    _delay_us(100);
}

} // namespace serial