#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include "SetupWDT.h"
#include "SetupEXT_IN_Interrupt.h"
#include "SetupTimer.h"
#include "Serial/UART.h"

volatile uint8_t inc_counter {0};
volatile bool ring_output {false};

void setup_GPIO() {
  DDRB |= (1 << DDB0);  // Set PB0 as output
  PORTB &= ~(1 << PORTB0); // Ensure PB0 is low initially
}

ISR(INT0_vect) {
  PORTB |= (1 << PORTB0);  // Set PB0 high
  inc_counter = 0;
}

ISR(TIMER1_COMPA_vect) {
  inc_counter += 1;
}

constexpr const uint16_t kTimer_compare_value{62499};
constexpr const uint8_t kRING_DURATION{6};

serial::Serial_parameters uart_parms = {
  serial::Communication_mode::kAsynchronous,
  serial::Asynchronous_mode::kNormal,
  serial::Baudrate::kBaud_9600,
  serial::StopBits::kOne,
  serial::DataBits::kEight,
  serial::Parity::kNone
};

int main (void) {
  setup_GPIO();

  external_pin_interrupt::setup_INT0_PullUpResistorFallingEdge();
  timer_interrupt::ctc_mode::setup_timer0_ctc_mode(timer_interrupt::Prescaler::DIV64, kTimer_compare_value);
  serial::UART uart(uart_parms);
  configure_wdt::timeout_1sec_reset_power::setup_WTD();

  sei();
  const char* motd = "smart haufe bell Version 0.1.0\r\n"; 
  while (1) {
    uart.send_string(motd);
    auto timer1_ctc_matches_interrupts_count = inc_counter;
    if (timer1_ctc_matches_interrupts_count >= kRING_DURATION && PORTB & (1 << PORTB0)) {
      PORTB &= ~(1 << PORTB0); // Set PB0 low
      inc_counter = 0;
    }
    wdt_reset();
    if (uart.is_read_data_available() > 0) {
      auto received_byte = uart.read_byte();
      wdt_reset();
      uart.send_byte(received_byte); // Echo back
    }
    wdt_reset();
  }
  return 0;
}