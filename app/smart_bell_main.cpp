/**
 * @file smart_bell_main.cpp
 * @brief SmartBell main application entry point.
 *
 * This is the main firmware for the SmartBell project.
 * It implements a network-connected doorbell with:
 * - DHCP for automatic IP configuration
 * - DNS for broker hostname resolution
 * - MQTT for event publishing and command subscription
 * - UART-based configuration interface
 *
 * Hardware:
 * - ATmega328P microcontroller
 * - W5500 Ethernet chip via SPI
 * - Doorbell button on INT0 (PD2)
 * - UART for debugging/configuration
 */

#ifdef __AVR__
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#endif

#include "App/SmartBellApp.h"
#include "Ethernet/W5500/W5500Interface.h"
#include "Serial/SPI.h"
#include "Serial/UART.h"
#include "SetupEXT_IN_Interrupt.h"
#include "SetupTimer.h"
#include "SetupWDT.h"
#include "System/TimerService.h"

// Hardware configuration
#ifdef __AVR__
static const constexpr uint8_t kSPI_CS_W5500 = (1 << PORTB2);
#else
static const constexpr uint8_t kSPI_CS_W5500 = 0x04;
#endif

// Global application instance (for ISR access)
static App::SmartBellApp* g_app = nullptr;

// Timer ISRs
#ifdef __AVR__
/**
 * @brief Timer0 Compare Match A ISR - 1ms tick
 */
ISR(TIMER0_COMPA_vect) { System::TimerService::on_1ms_tick(); }

/**
 * @brief Timer1 Compare Match A ISR - 1 second tick
 */
ISR(TIMER1_COMPA_vect) { System::TimerService::on_1s_tick(); }

/**
 * @brief External Interrupt 0 ISR - Doorbell button
 */
ISR(INT0_vect) {
  if (g_app != nullptr) {
    g_app->on_bell_interrupt();
  }
}
#endif

/**
 * @brief Setup Timer0 for 1ms interrupts.
 */
void setup_timer0_1ms() {
#ifdef __AVR__
  // CTC mode, prescaler 64
  // For 16MHz: 16000000 / 64 / 1000 = 250 counts for 1ms
  TCCR0A = (1 << WGM01);               // CTC mode
  TCCR0B = (1 << CS01) | (1 << CS00);  // Prescaler 64
  OCR0A = 249;                         // 250 counts (0-249)
  TIMSK0 = (1 << OCIE0A);              // Enable compare match interrupt
#endif
}

/**
 * @brief Setup Timer1 for 1 second interrupts.
 */
void setup_timer1_1sec() {
#ifdef __AVR__
  // CTC mode, prescaler 256
  // For 16MHz: 16000000 / 256 / 1 = 62500 counts for 1s
  TCCR1A = 0;
  TCCR1B = (1 << WGM12) | (1 << CS12);  // CTC mode, prescaler 256
  OCR1A = 62499;                        // 62500 counts (0-62499)
  TIMSK1 = (1 << OCIE1A);               // Enable compare match interrupt
#endif
}

int main() {
  // Disable WDT initially for setup
#ifdef __AVR__
  wdt_disable();
#endif

  // Enable interrupts
#ifdef __AVR__
  sei();
#endif

  // Setup UART for debugging
  serial::Serial_parameters uart_params;
  uart_params.baudrate = serial::Baudrate::kBaud_115200;
  uart_params.data_bits = serial::DataBits::kEight;
  uart_params.stop_bits = serial::StopBits::kOne;
  uart_params.parity_mode = serial::Parity::kNone;
  serial::UART uart(uart_params);

  uart.send_string("\r\n\r\n");
  uart.send_string("========================================\r\n");
  uart.send_string("  SmartBell v1.0.0\r\n");
  uart.send_string("  ATmega328P + W5500 + MQTT\r\n");
  uart.send_string("========================================\r\n");
  uart.send_string("\r\n");

  // Setup SPI for W5500
  serial::SPI_parameters spi_params;
  spi_params.mode = serial::SPI_mode::kMaster;
  spi_params.data_order = serial::SPI_data_order::kMSB;
  spi_params.clock_polarity = serial::SPI_clock_polarity::kIdleLow;
  spi_params.clock_phase = serial::SPI_clock_phase::kLeadingEdge;
  spi_params.clock_rate = serial::SPI_clock_rate::kFosc_4;
  serial::SPI spi(spi_params, kSPI_CS_W5500);

  uart.send_string("[MAIN] SPI initialized\r\n");

  // Setup W5500 callbacks
  Ethernet::W5500Callbacks w5500_callbacks;
#ifdef __AVR__
  w5500_callbacks.hard_reset = []() {
    // Toggle W5500 reset pin (if connected)
    // Assuming reset on PB1
    DDRB |= (1 << DDB1);
    PORTB &= ~(1 << PORTB1);
    for (volatile uint16_t i = 0; i < 10000; i++) {
    }
    PORTB |= (1 << PORTB1);
    for (volatile uint16_t i = 0; i < 10000; i++) {
    }
  };
  w5500_callbacks.chip_select = []() { PORTB &= ~kSPI_CS_W5500; };
  w5500_callbacks.chip_deselect = []() { PORTB |= kSPI_CS_W5500; };
#else
  w5500_callbacks.hard_reset = nullptr;
  w5500_callbacks.chip_select = nullptr;
  w5500_callbacks.chip_deselect = nullptr;
#endif

  // Create W5500 interface
  Ethernet::W5500Interface w5500(&spi, w5500_callbacks, &uart);
  w5500.init();

  uart.send_string("[MAIN] W5500 initialized\r\n");

  // Set MAC address
  Ethernet::MacAddress mac = {{0x02, 0x00, 0x00, 0x00, 0x00, 0x01}};
  w5500.set_MAC(&mac);

  uart.send_string("[MAIN] MAC address set\r\n");

  // Setup external interrupt for doorbell button (INT0, falling edge)
  external_pin_interrupt::setup_INT0_PullUpResistorFallingEdge();

  uart.send_string("[MAIN] External interrupt configured\r\n");

  // Setup timers
  setup_timer0_1ms();
  setup_timer1_1sec();

  uart.send_string("[MAIN] Timers configured\r\n");

  // Create and initialize application
  App::SmartBellApp app(&w5500, &uart);
  g_app = &app;
  app.init();

  // Enable watchdog with 8 second timeout for network operations
  configure_wdt::timeout_8sec_reset_power::setup_WDT();

  uart.send_string("[MAIN] Entering main loop\r\n");
  uart.send_string("\r\n");

  // Main loop
  while (1) {
    // Reset watchdog
    configure_wdt::reset();

    // Run application state machine
    app.loop();
  }

  return 0;
}
