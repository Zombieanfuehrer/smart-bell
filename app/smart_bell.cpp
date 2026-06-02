// Smart Bell Firmware with Lightweight ConfigManager
// Uses LightweightConfig for UART configuration (~200B vs ~1.7KB SRAM)

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <string.h>
#include <util/delay.h>

#include "Config/LightweightConfig.h"  // Lightweight config manager
#include "Ethernet/W5500/W5500Interface.h"
#include "MQTT/MinimalMQTT.h"
#include "Serial/SPI.h"
#include "Serial/UART.h"
#include "SetupEXT_IN_Interrupt.h"
#include "SetupTimer.h"
#include "SetupWDT.h"
#include "System/TimerService.h"

// ===== GLOBAL STATE =====
volatile uint8_t timer_counter = 0;
volatile bool button_pressed = false;
volatile bool ring_active = false;
volatile bool bell_enabled = true;
bool mqtt_ring_sent = false;

static serial::UART* g_uart = nullptr;
static MQTT::MinimalMQTT* g_mqtt_client = nullptr;
static Config::LightweightConfig* g_config = nullptr;
static Ethernet::W5500Interface* g_w5500 = nullptr;  // Global to avoid stack overflow

// UART command buffer
char cmd_buffer[32];
uint8_t cmd_index = 0;

// ===== HARDWARE PINS =====
static constexpr uint8_t kSPI_CS_W5500 = (1 << PORTB2);
static constexpr uint8_t kRESET_W5500 = (1 << PORTD4);
static constexpr uint8_t kRING_OUTPUT = (1 << PORTB0);

// ===== INTERRUPTS =====
ISR(INT0_vect) {
  if (!button_pressed && bell_enabled) {
    button_pressed = true;
    ring_active = true;
    timer_counter = 0;
    PORTB |= kRING_OUTPUT;  // Activate ring
  }
}

// Timer0 fires every 1ms → millis() counter
ISR(TIMER0_COMPA_vect) { System::TimerService::on_1ms_tick(); }

// Timer1 fires every 250ms → ring tick counter
ISR(TIMER1_COMPA_vect) { timer_counter++; }

// ===== SETUP FUNCTIONS =====
void setup_GPIO() {
  DDRB |= kRING_OUTPUT;  // Ring output
  PORTB &= ~kRING_OUTPUT;

  DDRB |= kSPI_CS_W5500;  // SPI CS
  PORTB |= kSPI_CS_W5500;

  DDRD |= kRESET_W5500;  // W5500 Reset
  PORTD |= kRESET_W5500;
}

int main() {
  // Disable WDT immediately - must happen before anything else.
  // After a WDT reset, WDT stays active. The bootloader (~300ms) +
  // CRT startup is well under the 8s timeout, so this is sufficient.
  MCUSR = 0;
  wdt_disable();
  setup_GPIO();

  // UART init
  serial::Serial_parameters uart_params = {serial::Communication_mode::kAsynchronous,
                                           serial::Asynchronous_mode::kNormal,
                                           serial::Baudrate::kBaud_19200,
                                           serial::StopBits::kOne,
                                           serial::DataBits::kEight,
                                           serial::Parity::kNone};
  serial::UART uart(uart_params);
  g_uart = &uart;

  // Enable interrupts early for UART TX to work
  sei();

  uart.send_string("\r\n=== Smart Bell with Config ===\r\n");
  uart.send_string("Type 'help' for commands\r\n\r\n");

  // Load configuration
  static Config::LightweightConfig config(uart);
  g_config = &config;

  config.load();  // Loads from EEPROM or uses defaults if invalid

  const Config::SmartBellConfig& cfg = config.config();

  // SPI init
  serial::SPI_parameters spi_params = {
      serial::SPI_mode::kMaster, serial::SPI_data_order::kMsb_first,
      serial::SPI_clock_polarity::kIdle_low, serial::SPI_clock_phase::kLeading,
      serial::SPI_clock_rate::k4mHz};
  static serial::SPI spi(spi_params, kSPI_CS_W5500);  // Static to avoid stack overflow

  // W5500 callbacks
  // chip_select disables SPIE so W5500 SPI callbacks can use direct SPDR
  // polling without ISR interference. chip_deselect restores SPIE.
  Ethernet::W5500Callbacks w5500_callbacks = {.hard_reset =
                                                  []() {
                                                    PORTD &= ~kRESET_W5500;
                                                    _delay_ms(1);
                                                    PORTD |= kRESET_W5500;
                                                    _delay_ms(10);
                                                  },
                                              .chip_select =
                                                  []() {
                                                    SPCR &= ~(1 << SPIE);  // polling mode
                                                    PORTB &= ~kSPI_CS_W5500;
                                                  },
                                              .chip_deselect =
                                                  []() {
                                                    PORTB |= kSPI_CS_W5500;
                                                    SPCR |= (1 << SPIE);  // restore ISR mode
                                                  }};

  // W5500 init
  uart.send_string("[ETH] Init W5500\r\n");

  // ALWAYS do hardware reset BEFORE creating W5500Interface object!
  // This ensures W5500 starts in a clean state after ATmega reset
  PORTB |= kSPI_CS_W5500;  // CS high (deselect)
  PORTD &= ~kRESET_W5500;  // Reset LOW (active)
  _delay_ms(50);
  wdt_reset();
  PORTD |= kRESET_W5500;  // Reset HIGH (inactive)
  _delay_ms(100);
  wdt_reset();
  _delay_ms(100);  // Total 200ms stabilization (datasheet: ~160ms)

  // Use STATIC to avoid stack overflow! W5500Interface is large (~200+ bytes)
  static Ethernet::W5500Interface w5500(&spi, w5500_callbacks);
  g_w5500 = &w5500;

  w5500.init();
  uart.send_string("[ETH] Init OK\r\n");

  // Network config from LightweightConfig
  Ethernet::MacAddress mac;
  Ethernet::IpAddress ip;
  Ethernet::SubnetMask subnet;
  Ethernet::GatewayAddress gateway;

  memcpy(mac.addr, cfg.mac, 6);
  memcpy(ip.addr, cfg.device_ip, 4);
  memcpy(subnet.addr, cfg.subnet, 4);
  memcpy(gateway.addr, cfg.gateway, 4);

  w5500.set_network_config(&mac, &ip, &subnet, &gateway);
  uart.send_string("[ETH] Network OK\r\n");

  // MQTT init from config
  static MQTT::MinimalMQTT mqtt_client_instance{&uart};
  g_mqtt_client = &mqtt_client_instance;

  MQTT::Config mqtt_config;
  memcpy(mqtt_config.broker_ip, cfg.broker_ip, 4);
  mqtt_config.broker_port = cfg.broker_port;
  strncpy(mqtt_config.client_id, cfg.client_id, MQTT::kMaxClientIdLength);
  mqtt_config.client_id[MQTT::kMaxClientIdLength - 1] = '\0';
  mqtt_config.use_auth = false;
  mqtt_config.keepalive = 60;

  // Store MQTT config, connect only if broker IP is non-zero
  bool mqtt_configured =
      (cfg.broker_ip[0] | cfg.broker_ip[1] | cfg.broker_ip[2] | cfg.broker_ip[3]) != 0;
  if (mqtt_configured) {
    uart.send_string("[MQTT] Connecting...\r\n");
    mqtt_client_instance.connect(mqtt_config);
  } else {
    uart.send_string("[MQTT] No broker configured - use 'br' to set\r\n");
  }

  // Setup interrupts
  external_pin_interrupt::setup_INT0_PullUpResistorFallingEdge();
  // Timer0: 1ms tick for millis() → F_CPU/(N*(OCR0A+1)) = 16MHz/(64*250) = 1000 Hz
  timer_interrupt::ctc_mode::setup_timer0_1ms();
  // Timer1: 250ms tick for ring timer → OCR1A=62499, DIV64
  timer_interrupt::ctc_mode::setup_timer0_ctc_mode(timer_interrupt::Prescaler::DIV64, 62499);

  uart.send_string("[SYS] Started\r\n\r\n");

  // WDT disabled - 8s is already the ATmega max, and init + MQTT connect can exceed it.
  // Re-enable WDT here once system is fully stable.

  uint32_t last_millis = 0;
  static const char kTopicRing[] = "home/bell/ring";

  // Main loop
  while (1) {
    // MQTT keepalive / state machine
    g_mqtt_client->loop();

    // MQTT connect: only triggered after explicit 'save' command
    if (g_config->consume_save_flag()) {
      const Config::SmartBellConfig& live_cfg = g_config->config();
      bool has_broker = (live_cfg.broker_ip[0] | live_cfg.broker_ip[1] | live_cfg.broker_ip[2] |
                         live_cfg.broker_ip[3]) != 0;
      if (has_broker) {
        // Static to avoid 81-byte stack allocation (2KB SRAM limit!)
        static MQTT::Config reconnect_cfg;
        memcpy(reconnect_cfg.broker_ip, live_cfg.broker_ip, 4);
        reconnect_cfg.broker_port = live_cfg.broker_port;
        strncpy(reconnect_cfg.client_id, live_cfg.client_id, MQTT::kMaxClientIdLength);
        reconnect_cfg.client_id[MQTT::kMaxClientIdLength - 1] = '\0';
        reconnect_cfg.use_auth = false;
        reconnect_cfg.keepalive = 60;
        g_mqtt_client->connect(reconnect_cfg);
      }
    }

    // Process UART commands - drain entire RX buffer per iteration
    while (uart.is_read_data_available()) {
      char c = uart.read_byte();

      if (c == '\r' || c == '\n') {
        if (cmd_index > 0) {
          cmd_buffer[cmd_index] = '\0';
          uart.send_string("\r\n");

          if (!g_config->process_command(cmd_buffer)) {
            uart.send_string("[ERR] Unknown cmd\r\n");
          }

          cmd_index = 0;
        }
      } else if (c == '\b' || c == 0x7F) {
        // Backspace / DEL: erase last character
        if (cmd_index > 0) {
          cmd_index--;
          uart.send_string("\b \b");  // move back, overwrite with space, move back again
        }
      } else if (cmd_index < 31) {
        cmd_buffer[cmd_index++] = c;
        char echo[2] = {c, '\0'};
        uart.send_string(echo);  // Echo
      }
    }

    // Ring timer (6 ticks × 250ms = 1.5 seconds)
    if (ring_active && timer_counter >= 6) {
      ring_active = false;
      mqtt_ring_sent = false;
      PORTB &= ~kRING_OUTPUT;  // Deactivate ring
      uart.send_string("[BELL] Ring ended\r\n");
    }

    // MQTT publish on button press
    if (button_pressed && !mqtt_ring_sent) {
      const char* payload = "1";
      if (g_mqtt_client->publish(kTopicRing, reinterpret_cast<const uint8_t*>(payload), 1)) {
        mqtt_ring_sent = true;
        uart.send_string("[MQTT] Published\r\n");
      }
    }

    // Reset button flag after ring ends
    if (!ring_active) {
      button_pressed = false;
    }

    // Minimal delay
    uint32_t current_millis = System::TimerService::millis();
    if (current_millis - last_millis > 100) {
      last_millis = current_millis;
      _delay_ms(10);
    }
  }

  return 0;
}
