#ifndef TESTS_ETHERNET_TESTCOMPAT_H_
#define TESTS_ETHERNET_TESTCOMPAT_H_

// Kompatibilitäts-Header für Unit-Tests
// Ersetzt AVR-spezifische Funktionen durch Dummy-Implementierungen

// Dummy für AVR delay
#ifndef _delay_ms
#define _delay_ms(ms) \
  do {                \
  } while (0)
#endif

// Dummy für AVR interrupt enable
#ifndef sei
#define sei() \
  do {        \
  } while (0)
#endif

// AVR SPI Register-Definitionen (Dummy-Werte für Tests)
#ifndef MSTR
#define MSTR 4
#define DORD 5
#define CPOL 3
#define CPHA 2
#define SPR1 0
#define SPR0 1
#define SPI2X 0
#define DDB3 3
#define DDB4 4
#define DDB5 5
#endif

// AVR UART Register-Definitionen (Dummy-Werte für Tests)
#ifndef UMSEL00
#define UMSEL00 0
#define UMSEL01 1
#define U2X0 1
#define USBS0 3
#define UCSZ00 1
#define UPM00 4
#endif

#endif  // TESTS_ETHERNET_TESTCOMPAT_H_
