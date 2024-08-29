#include <avr/io.h>
#include <avr/interrupt.h>


#include "SetupEXT_IN_Interrupt.h"


namespace external_pin_interrupt {

void setup_INT0_PullUpResistorFallingEdge(void) {
    DDRD &= ~(1 << DDD2);   // Set PD2 (INT0) as input
    PORTD |= (1 << PORTD2); // Enable pull-up resistor on PD2


    EICRA |= (1 << ISC01);  // Set ISC01 bit for falling edge
    EICRA &= ~(1 << ISC00); // Clear ISC00 bit for falling edge

    EIMSK |= (1 << INT0);   // Enable INT0 interrupt
}

}  // external_pin_interrupt

