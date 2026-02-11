#ifdef __AVR__
#include <avr/io.h>
#include <avr/interrupt.h>
#endif

#include "SetupEXT_IN_Interrupt.h"

namespace external_pin_interrupt {

void setup_INT0(EdgeType edge) {
#ifdef __AVR__
  // Set PD2 (INT0) as input with pull-up
  DDRD &= ~(1 << DDD2);
  PORTD |= (1 << PORTD2);

  // Configure edge detection
  switch (edge) {
    case EdgeType::kFalling:
      EICRA |= (1 << ISC01);
      EICRA &= ~(1 << ISC00);
      break;
    case EdgeType::kRising:
      EICRA |= (1 << ISC01) | (1 << ISC00);
      break;
    case EdgeType::kAnyChange:
      EICRA &= ~(1 << ISC01);
      EICRA |= (1 << ISC00);
      break;
  }

  // Enable INT0 interrupt
  EIMSK |= (1 << INT0);
#else
  (void)edge;
#endif
}

void setup_INT1(EdgeType edge) {
#ifdef __AVR__
  // Set PD3 (INT1) as input with pull-up
  DDRD &= ~(1 << DDD3);
  PORTD |= (1 << PORTD3);

  // Configure edge detection
  switch (edge) {
    case EdgeType::kFalling:
      EICRA |= (1 << ISC11);
      EICRA &= ~(1 << ISC10);
      break;
    case EdgeType::kRising:
      EICRA |= (1 << ISC11) | (1 << ISC10);
      break;
    case EdgeType::kAnyChange:
      EICRA &= ~(1 << ISC11);
      EICRA |= (1 << ISC10);
      break;
  }

  // Enable INT1 interrupt
  EIMSK |= (1 << INT1);
#else
  (void)edge;
#endif
}

void setup_both_interrupts(EdgeType int0_edge, EdgeType int1_edge) {
  setup_INT0(int0_edge);
  setup_INT1(int1_edge);
}

bool read_INT0_state() {
#ifdef __AVR__
  return (PIND & (1 << PIND2)) != 0;
#else
  return true;
#endif
}

bool read_INT1_state() {
#ifdef __AVR__
  return (PIND & (1 << PIND3)) != 0;
#else
  return true;
#endif
}

void setup_INT0_PullUpResistorFallingEdge(void) {
  setup_INT0(EdgeType::kFalling);
}

}  // namespace external_pin_interrupt

