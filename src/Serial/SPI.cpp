#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <stdint.h>
#include <util/delay.h>

#include "Serial/SPI.h"

volatile static uint8_t rx_data = 0;
volatile static uint8_t Rx_buffer_[serial::SPI::kRX_buffer_size] = {0};
volatile static uint16_t Rx_buffer_head_ = 0;
volatile static uint16_t Rx_buffer_tail_ = 0;
volatile static uint8_t tx_rx_done = 0;

ISR(SPI_STC_vect) {
  if (SPSR & (1 << WCOL)) {
    // Write collision
    Rx_buffer_[Rx_buffer_tail_] = SPDR; // Read out of the Rx Buffer
    Rx_buffer_[Rx_buffer_tail_] = 0;    // Clear the Rx
    tx_rx_done = 1;
  } else {
    Rx_buffer_[Rx_buffer_tail_] = SPDR;
    Rx_buffer_tail_++;
    if (Rx_buffer_tail_ >= serial::SPI::kRX_buffer_size) {
      Rx_buffer_tail_ = 0;
    }
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
  PORTB &= ~(1 << slave_select_);
  _delay_us(100);
  this->send_(byte);
  PORTB |= (1 << slave_select_);
  _delay_us(100);
}

void SPI::send_bytes(const uint8_t *const bytes, const uint16_t length) {
  PORTB &= ~(1 << slave_select_);
  _delay_us(100);
  for (uint16_t i = 0; i < length; i++) {
    this->send_(bytes[i]);
  }
  PORTB |= (1 << slave_select_);
  _delay_us(100);
}

void SPI::send_string(const char *string) {
  PORTB &= ~(1 << slave_select_);
  _delay_us(100);
  uint16_t nByte = 0;
  while (string[nByte] != '\0') {
    this->send_(string[nByte]);
    nByte++;
  }
  PORTB |= (1 << slave_select_);
  _delay_us(100);
}

void SPI::send_(const uint8_t byte) {
  tx_rx_done = 0;
  SPDR = byte;
  while (!tx_rx_done)
    ;
}

uint16_t SPI::is_read_data_available() const { 
  return (Rx_buffer_head_ != Rx_buffer_tail_);
}

uint8_t SPI::read_byte() { 
  if (Rx_buffer_head_ == Rx_buffer_tail_) {
    return 0; // Buffer is empty
  }
  auto byte = Rx_buffer_[Rx_buffer_head_];
  Rx_buffer_head_++;
  if (Rx_buffer_head_ >= SPI::kRX_buffer_size) {
    Rx_buffer_head_ = 0;
  }
  return byte;
}


} // namespace serial