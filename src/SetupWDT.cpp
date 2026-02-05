#ifdef __AVR__
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#endif

#include "SetupWDT.h"

namespace configure_wdt {
namespace timeout_1sec_reset_power {
void setup_WDT(void) {
#ifdef __AVR__
  cli();
  WDTCSR |= (1 << WDCE) | (1 << WDE);
  // WDP2 | WDP1 | WDP0 = 1s timeout, WDIE = interrupt enable
  WDTCSR = (1 << WDIE) | (1 << WDP2) | (1 << WDP1) | (0 << WDP0);
  sei();
  wdt_reset();
#endif
}
}  // namespace timeout_1sec_reset_power

namespace timeout_8sec_reset_power {
void setup_WDT(void) {
#ifdef __AVR__
  cli();
  WDTCSR |= (1 << WDCE) | (1 << WDE);
  // WDP3 | WDP0 = 8s timeout, WDIE = interrupt enable
  WDTCSR = (1 << WDIE) | (1 << WDP3) | (1 << WDP0);
  sei();
  wdt_reset();
#endif
}
}  // namespace timeout_8sec_reset_power

void pause(void) {
#ifdef __AVR__
  wdt_disable();
#endif
}

void resume(void) {
#ifdef __AVR__
  timeout_8sec_reset_power::setup_WDT();
#endif
}

void reset(void) {
#ifdef __AVR__
  wdt_reset();
#endif
}

void disable(void) {
#ifdef __AVR__
  wdt_disable();
#endif
}
}  // namespace configure_wdt