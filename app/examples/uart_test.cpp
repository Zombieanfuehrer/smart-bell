#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include "Serial/UART.h"

int main() {
  // 1. Watchdog sicherheitshalber deaktivieren
  MCUSR = 0;
  wdt_disable();

  // 2. Optisches Feedback: LED-Pin (PORTB0) als Ausgang definieren
  DDRB |= (1 << PORTB0);
  PORTB &= ~(1 << PORTB0);

  // 3. Ihre UART-Parameter für den Test konfigurieren
  serial::Serial_parameters params;
  params.communication_mode = serial::Communication_mode::kAsynchronous;
  params.asynchronous_mode = serial::Asynchronous_mode::kNormal;
  params.baudrate = serial::Baudrate::kBaud_19200;
  params.stop_bits = serial::StopBits::kOne;
  params.data_bits = serial::DataBits::kEight;
  params.parity_mode = serial::Parity::kNone;

  // 4. Instanziierung Ihrer UART-Klasse
  serial::UART test_uart(params);

  // 5. Globale Interrupts aktivieren (wichtig für Ihren RX-Interrupt-Vektor)
  sei();

  // 6. Test-Schleife
  while (1) {
    // LED einschalten (Sende-Anzeige)
    PORTB |= (1 << PORTB0);

    // Test-String über Ihre Klassen-Implementierung senden
    test_uart.send_string("INTEGRATION-TEST: UART-Klasse sendet erfolgreich!\r\n");

    // LED ausschalten
    PORTB &= ~(1 << PORTB0);

    // Echo-Test: Wenn Zeichen im RX-Buffer liegen, direkt zurückschicken
    if (test_uart.is_read_data_available()) {
      uint8_t echo_byte = test_uart.read_byte();
      test_uart.send_string("ECHO: ");
      test_uart.send(echo_byte);
      test_uart.send_string("\r\n");
    }

    // 1 Sekunde Pause
    _delay_ms(1000);
  }

  return 0;
}
