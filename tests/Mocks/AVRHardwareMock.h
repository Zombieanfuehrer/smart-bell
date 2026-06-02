#ifndef TESTS_MOCKS_AVRHARDWAREMOCK_H_
#define TESTS_MOCKS_AVRHARDWAREMOCK_H_

/**
 * @file AVRHardwareMock.h
 * @brief Mock implementation for AVR hardware registers and functions
 *
 * This header provides mock implementations of AVR-specific functionality
 * to enable unit testing on Linux/x86_64 platforms without actual hardware.
 *
 * Usage: Include this before any AVR headers when building tests.
 */

#include <cstdint>
#include <cstring>
#include <map>

// ============================================================================
// AVR EEPROM Mock
// ============================================================================

namespace mock {

class EEPROMMock {
 public:
  static EEPROMMock& instance() {
    static EEPROMMock instance;
    return instance;
  }

  void write_byte(uint16_t address, uint8_t value) { memory_[address] = value; }

  uint8_t read_byte(uint16_t address) const {
    auto it = memory_.find(address);
    return (it != memory_.end()) ? it->second : 0xFF;
  }

  void write_block(const void* data, void* address_ptr, size_t size) {
    const uint8_t* src = static_cast<const uint8_t*>(data);
    uint16_t addr = reinterpret_cast<uintptr_t>(address_ptr);
    for (size_t i = 0; i < size; i++) {
      memory_[addr + i] = src[i];
    }
  }

  void read_block(void* data, const void* address_ptr, size_t size) {
    uint8_t* dst = static_cast<uint8_t*>(data);
    uint16_t addr = reinterpret_cast<uintptr_t>(address_ptr);
    for (size_t i = 0; i < size; i++) {
      auto it = memory_.find(addr + i);
      dst[i] = (it != memory_.end()) ? it->second : 0xFF;
    }
  }

  void clear() { memory_.clear(); }

 private:
  EEPROMMock() = default;
  std::map<uint16_t, uint8_t> memory_;
};

}  // namespace mock

// EEPROM function replacements
inline void eeprom_write_byte(uint8_t* address_ptr, uint8_t value) {
  uint16_t addr = reinterpret_cast<uintptr_t>(address_ptr);
  mock::EEPROMMock::instance().write_byte(addr, value);
}

inline uint8_t eeprom_read_byte(const uint8_t* address_ptr) {
  uint16_t addr = reinterpret_cast<uintptr_t>(address_ptr);
  return mock::EEPROMMock::instance().read_byte(addr);
}

inline void eeprom_write_block(const void* data, void* address, size_t size) {
  mock::EEPROMMock::instance().write_block(data, address, size);
}

inline void eeprom_read_block(void* data, const void* address, size_t size) {
  mock::EEPROMMock::instance().read_block(data, address, size);
}

inline void eeprom_update_byte(uint8_t* address, uint8_t value) {
  eeprom_write_byte(address, value);
}

inline void eeprom_update_block(const void* data, void* address, size_t size) {
  eeprom_write_block(data, address, size);
}

// ============================================================================
// AVR Delay Functions
// ============================================================================

inline void _delay_ms(double ms) {
  // No-op for tests (or use std::this_thread::sleep_for if needed)
  (void)ms;
}

inline void _delay_us(double us) { (void)us; }

// ============================================================================
// AVR Interrupt Functions
// ============================================================================

inline void sei() {
  // Global interrupt enable - no-op in tests
}

inline void cli() {
  // Global interrupt disable - no-op in tests
}

// ============================================================================
// AVR SPI Register Definitions (Dummy)
// ============================================================================

// SPCR - SPI Control Register
#define MSTR 4
#define DORD 5
#define CPOL 3
#define CPHA 2
#define SPR1 1
#define SPR0 0
#define SPE 6
#define SPIE 7

// SPSR - SPI Status Register
#define SPI2X 0
#define WCOL 6
#define SPIF 7

// DDR/PORT definitions
#define DDB2 2
#define DDB3 3
#define DDB4 4
#define DDB5 5

// ============================================================================
// AVR UART Register Definitions (Dummy)
// ============================================================================

// UCSR0A - USART Control and Status Register A
#define MPCM0 0
#define U2X0 1
#define UPE0 2
#define DOR0 3
#define FE0 4
#define UDRE0 5
#define TXC0 6
#define RXC0 7

// UCSR0B - USART Control and Status Register B
#define TXB80 0
#define RXB80 1
#define UCSZ02 2
#define TXEN0 3
#define RXEN0 4
#define UDRIE0 5
#define TXCIE0 6
#define RXCIE0 7

// UCSR0C - USART Control and Status Register C
#define UCPOL0 0
#define UCSZ00 1
#define UCSZ01 2
#define USBS0 3
#define UPM00 4
#define UPM01 5
#define UMSEL00 6
#define UMSEL01 7

// ============================================================================
// AVR Watchdog/Timer Definitions
// ============================================================================

#define WDTO_15MS 0
#define WDTO_30MS 1
#define WDTO_60MS 2
#define WDTO_120MS 3
#define WDTO_250MS 4
#define WDTO_500MS 5
#define WDTO_1S 6
#define WDTO_2S 7
#define WDTO_4S 8
#define WDTO_8S 9

inline void wdt_enable(uint8_t timeout) { (void)timeout; }

inline void wdt_disable() {
  // No-op
}

inline void wdt_reset() {
  // No-op
}

// ============================================================================
// AVR PROGMEM Handling
// ============================================================================

#ifndef PROGMEM
#define PROGMEM
#endif

#ifndef PSTR
#define PSTR(s) (s)
#endif

#ifndef pgm_read_byte
#define pgm_read_byte(addr) (*(const uint8_t*)(addr))
#endif

#ifndef strlen_P
#define strlen_P(s) strlen(s)
#endif

#ifndef strcpy_P
#define strcpy_P(dest, src) strcpy(dest, src)
#endif

#ifndef strcmp_P
#define strcmp_P(s1, s2) strcmp(s1, s2)
#endif

// ============================================================================
// Atomic Operations
// ============================================================================

#define ATOMIC_BLOCK(type) if (true)
#define ATOMIC_RESTORESTATE
#define ATOMIC_FORCEON

#endif  // TESTS_MOCKS_AVRHARDWAREMOCK_H_
