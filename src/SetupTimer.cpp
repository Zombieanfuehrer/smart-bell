#include <avr/io.h>
#include <avr/interrupt.h>

#include "SetupTimer.h"

namespace timer_interrupt {
  namespace ctc_mode {
    void setup_timer0_ctc_mode(const Prescaler prescaler, const uint16_t ocr1a_value) {
      cli();
      // Set CTC mode (Clear Timer on Compare Match)
      TCCR1B |= (1 << WGM12);
      // Set prescaler to 64
      TCCR1B |= static_cast<uint8_t>(prescaler);
      // Set Compare Match value for 250ms interrupt
      OCR1A = ocr1a_value;
      // Enable Timer1 Compare Match A interrupt
      TIMSK1 |= (1 << OCIE1A);
      sei();
    }
  }  // namespace ctc_mode
}  // namespace timer_interrupt