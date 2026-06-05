#ifdef __AVR__
#include <avr/interrupt.h>
#include <avr/io.h>
#endif

#include "SetupTimer.h"

namespace timer_interrupt {
namespace ctc_mode {
void setup_timer1_ctc_mode(const Prescaler prescaler, const uint16_t ocr1a_value) {
  cli();

  TCCR1A = 0;
  // Set CTC mode
  TCCR1B = (1 << WGM12);
  TCCR1B |= static_cast<uint8_t>(prescaler);
  // Set Compare Match value for <val> interrupt
  OCR1A = ocr1a_value;
  // Enable Timer1 Compare Match A interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei();
}

void setup_timer0_1ms() {
  cli();

  TIMSK0 = 0;
  // Timer0 CTC mode: WGM01=1, WGM00=0
  TCCR0A = (1 << WGM01);
  // Prescaler DIV64
  TCCR0B = (1 << CS01) | (1 << CS00);
  // OCR0A = (F_CPU / (N * f_target)) - 1 = (16000000 / (64 * 1000)) - 1 = 249
  OCR0A = 249;
  // Enable Timer0 Compare Match A interrupt
  TIMSK0 |= (1 << OCIE0A);
  sei();
}
}  // namespace ctc_mode
}  // namespace timer_interrupt