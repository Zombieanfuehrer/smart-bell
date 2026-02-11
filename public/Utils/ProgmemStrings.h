#ifndef PUBLIC_UTILS_PROGMEM_STRINGS_H_
#define PUBLIC_UTILS_PROGMEM_STRINGS_H_

#ifdef __AVR__
#include <avr/pgmspace.h>
#define PSTR_STORED(s) PSTR(s)
#else
// For non-AVR builds, PROGMEM strings are just regular strings
#define PSTR_STORED(s) (s)
#define PROGMEM
#define pgm_read_byte(addr) (*(addr))
#endif

#include <stdint.h>
#include "Serial/Interface.h"

namespace Utils {

/**
 * @brief Print a PROGMEM string to UART.
 * @param uart UART interface
 * @param pstr Pointer to PROGMEM string
 */
inline void print_P(serial::Interface* uart, const char* pstr) {
  if (uart == nullptr)
    return;
#ifdef __AVR__
  char c;
  while ((c = pgm_read_byte(pstr++)) != '\0') {
    uart->send(static_cast<uint8_t>(c));
  }
#else
  uart->send_string(pstr);
#endif
}

/**
 * @brief Print a PROGMEM string followed by newline.
 * @param uart UART interface
 * @param pstr Pointer to PROGMEM string
 */
inline void println_P(serial::Interface* uart, const char* pstr) {
  print_P(uart, pstr);
  if (uart != nullptr) {
    uart->send_string("\r\n");
  }
}

}  // namespace Utils

#endif  // PUBLIC_UTILS_PROGMEM_STRINGS_H_
