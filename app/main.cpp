#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include "SetupWDT.h"
#include "SetupEXT_IN_Interrupt.h"
#include "SetupTimer.h"
#include "Serial/UART.h"
#include "Serial/SPI.h"

volatile uint8_t inc_counter {0};
volatile bool ring_output {false};

void setup_GPIO() {
  DDRB |= (1 << DDB0);  // Set PB0 as output
  PORTB &= ~(1 << PORTB0); // Ensure PB0 is low initially

  DDRB |= (1 << DDB2);  // Set PB2 as output
  PORTB &= ~(1 << PORTB2); // Ensure PB2 is low initially
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

serial::SPI_parameters spi_parms = {
  serial::SPI_mode::kMaster,
  serial::SPI_data_order::kMsb_first,
  serial::SPI_clock_polarity::kIdle_low,
  serial::SPI_clock_phase::kLeading,
  serial::SPI_clock_rate::k4mHz
};
static const constexpr uint8_t kSPI_CS_W5500 = (1 << PORTB2);

int main (void) {
  setup_GPIO();
  serial::SPI spi(spi_parms, kSPI_CS_W5500);
  spi.send(0x01);
  external_pin_interrupt::setup_INT0_PullUpResistorFallingEdge();
  timer_interrupt::ctc_mode::setup_timer0_ctc_mode(timer_interrupt::Prescaler::DIV64, kTimer_compare_value);
  serial::UART uart(uart_parms);
  configure_wdt::timeout_1sec_reset_power::setup_WTD();

  sei();
  const char* motd = "smart haufe bell Version 0.1.0\r\n";
  bool motd_sent = false;
  while (1) {
    if(!motd_sent) {
      uart.send_string(motd);
      motd_sent = true;
    }
    auto timer1_ctc_matches_interrupts_count = inc_counter;
    if (timer1_ctc_matches_interrupts_count >= kRING_DURATION && PORTB & (1 << PORTB0)) {
      PORTB &= ~(1 << PORTB0); // Set PB0 low
      inc_counter = 0;
    }
    wdt_reset();
    if (uart.is_read_data_available() > 0) {
      auto received_byte = uart.read_byte();
      wdt_reset();
      uart.send(received_byte); // Echo back
    }
    wdt_reset();
  }
  return 0;
}