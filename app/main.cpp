#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include "SetupWDT.h"
#include "SetupEXT_IN_Interrupt.h"
#include "SetupTimer.h"

volatile uint8_t inc_counter {0};
volatile bool ring_output {false};

void setup_GPIO() {
  DDRB |= (1 << DDB0);  // Set PB0 as output
  PORTB &= ~(1 << PORTB0); // Ensure PB0 is low initially
}

ISR(INT0_vect) {
  ring_output = true;
  inc_counter = 0;
}

ISR(TIMER1_COMPA_vect) {
  inc_counter += 1;
}

constexpr const uint16_t kOCR1A_VALUE{62499};
int main () {
  setup_GPIO();
  configure_wdt::timeout_1sec_reset_power::setup_WTD();
  external_pin_interrupt::setup_INT0_PullUpResistorFallingEdge();
  timer_interrupt::ctc_mode::setup_timer0_ctc_mode(timer_interrupt::Prescaler::DIV64, kOCR1A_VALUE);

  while (1) {

    if (inc_counter >= 80 && ring_output) {
      PORTB &= ~(1 << PORTB0); // Set PB0 low
        ring_output = false;
        inc_counter = 0;
    }

    if (ring_output && inc_counter < 80 && !(PORTB & (1 << PORTB0))) {
      PORTB |= (1 << PORTB0);  // Set PB0 high
    }
    wdt_reset();
  }
  return 0;
}