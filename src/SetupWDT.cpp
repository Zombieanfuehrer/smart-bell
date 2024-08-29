#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include "SetupWDT.h"

namespace configure_wdt {
  namespace timeout_1sec_reset_power{
    void setup_WTD(void) {
        cli();
        WDTCSR |= (1<<WDCE) | (1<<WDE);

        WDTCSR = (1<<WDIE) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0); // 1s
        sei();
        wdt_reset();
    }
  }  // namespace timeout_1sec_reset_power
}  // namespace configure_wdt


