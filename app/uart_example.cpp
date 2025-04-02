#ifdef __AVR__
#include <avr/io.h>
#include <util/delay.h>
#include <avr/wdt.h>
#endif

#include "Serial/UART.h"

serial::Serial_parameters uart_parms = {
  serial::Communication_mode::kAsynchronous,
  serial::Asynchronous_mode::kNormal,
  serial::Baudrate::kBaud_9600,
  serial::StopBits::kOne,
  serial::DataBits::kEight,
  serial::Parity::kNone
};
serial::UART uart(uart_parms);

void check_reset_cause_early() {
  if (MCUSR & (1 << WDRF)) {
      // Watchdog-Reset
      PORTB &= ~(1 << PB0);  // LED aus
      _delay_ms(500);        // Verzögerung sichtbar machen
  } else if (MCUSR & (1 << BORF)) {
      // Brown-Out-Reset
      PORTB &= ~(1 << PB0);  // LED aus
      _delay_ms(500);
  } else if (MCUSR & (1 << EXTRF)) {
      // Externer Reset
      PORTB &= ~(1 << PB0);  // LED aus
      _delay_ms(500);
  } else if (MCUSR & (1 << PORF)) {
      // Power-On-Reset
      PORTB &= ~(1 << PB0);  // LED aus
      _delay_ms(500);
  }
  MCUSR = 0;  // Reset-Ursache löschen
}

int main(void) {
  wdt_disable();  // Watchdog deaktivieren
  check_reset_cause_early();
  sei(); 

  DDRB |= (1 << PB0);  
  PORTB |= (1 << PB0);


  // Test 1: Senden einer Zeichenkette
  uart.send_string("Test 1: Sending a string...\n\r");
  uart.send_string("Hello, UART!\n\r");

  // Test 2: Senden eines einzelnen Zeichens
  uart.send_string("Test 2: Sending a single character...\n\r");
  uart.send('A');
  uart.send('\n');
  uart.send('\r');

  // Test 3: Empfang und Echo von Daten
  uart.send_string("Test 3: Echo received data...\n\r");
  uart.send_string("Please type something:\n\r");
  while (!uart.is_read_data_available()) {
    // Warten, bis Daten verfügbar sind
  }
  char received_data = uart.read_byte();
  uart.send_string("You typed: ");
  uart.send(received_data);
  uart.send('\n');
  uart.send('\r');

  // Test 4: Wiederholtes Senden und Empfangen
  uart.send_string("Test 4: Repeated send and receive...\n\r");
  for (int i = 0; i < 5; ++i) {
    uart.send_string("Iteration: ");
    uart.send('0' + i);  // Senden der Iterationsnummer
    uart.send('\n');
    uart.send('\r');

    uart.send_string("Please type something:\n\r");
    while (!uart.is_read_data_available()) {
      // Warten, bis Daten verfügbar sind
    }
    char received_data = uart.read_byte();
    uart.send_string("You typed: ");
    uart.send(received_data);
    uart.send('\n');
    uart.send('\r');

    _delay_ms(500);
  }

  // Test 5: Endlosschleife mit regulärem Betrieb
  uart.send_string("Test 5: Entering regular operation mode...\n\r");

  return 0;
}