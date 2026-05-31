#include "System/TimerService.h"

#ifdef __AVR__
#include <avr/interrupt.h>
#endif

// Forward declarations for ioLibrary timer handlers
#ifdef __AVR__
extern "C" {
void MilliTimer_Handler(void);
}
#endif

namespace System {

volatile uint32_t TimerService::millis_counter_ = 0;
volatile uint32_t TimerService::seconds_counter_ = 0;

void TimerService::on_1ms_tick() {
  millis_counter_++;
#ifdef __AVR__
  // Call MQTT millisecond timer handler
  MilliTimer_Handler();
#endif
}

void TimerService::on_1s_tick() {
  seconds_counter_++;
  // Note: DHCP/DNS removed for static IP mode
}

uint32_t TimerService::millis() {
  uint32_t m;
#ifdef __AVR__
  // Disable interrupts to ensure atomic read of 32-bit value
  uint8_t oldSREG = SREG;
  cli();
#endif
  m = millis_counter_;
#ifdef __AVR__
  SREG = oldSREG;
#endif
  return m;
}

uint32_t TimerService::seconds() {
  uint32_t s;
#ifdef __AVR__
  uint8_t oldSREG = SREG;
  cli();
#endif
  s = seconds_counter_;
#ifdef __AVR__
  SREG = oldSREG;
#endif
  return s;
}

bool TimerService::has_elapsed_ms(uint32_t start_time, uint32_t timeout_ms) {
  return (millis() - start_time) >= timeout_ms;
}

bool TimerService::has_elapsed_sec(uint32_t start_time, uint32_t timeout_sec) {
  return (seconds() - start_time) >= timeout_sec;
}

void TimerService::reset() {
#ifdef __AVR__
  uint8_t oldSREG = SREG;
  cli();
#endif
  millis_counter_ = 0;
  seconds_counter_ = 0;
#ifdef __AVR__
  SREG = oldSREG;
#endif
}

}  // namespace System
