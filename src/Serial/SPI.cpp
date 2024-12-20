#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <stdint.h>

#include "Serial/SPI.h"

namespace serial {

SPI::SPI(const SPI_parameters &parameters) {

  // Set MOSI and SCK output, all others input
  DDRB |= kSCK | kMOSI | kSS;


  // Enable SPI, Master, set clock rate fck/16
  SPCR = static_cast<uint8_t>(parameters.spi_mode) |
         static_cast<uint8_t>(parameters.data_order) |
         static_cast<uint8_t>(parameters.clock_polarity) |
         static_cast<uint8_t>(parameters.clock_phase) |
         static_cast<uint8_t>(parameters.clock_rate) |
         (1 << SPE);
}

} // namespace serial