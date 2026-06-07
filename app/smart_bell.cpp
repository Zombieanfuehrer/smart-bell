#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <string.h>
#include <util/delay.h>

#include "Config/LightweightConfig.h"
#include "Ethernet/W5500/W5500Interface.h"
#include "MQTT/MinimalMQTT.h"
#include "Serial/SPI.h"
#include "Serial/UART.h"
#include "SetupTimer.h"
#include "SetupWDT.h"
#include "System/TimerService.h"

// ===== HARDWARE KONSTANTEN =====
static constexpr uint8_t kSPI_CS_W5500 = (1 << PORTB2);
static constexpr uint8_t kRESET_W5500 = (1 << PORTD4);

// ===== CHIME STATE MACHINE STRUCT =====
struct ChimeState {
  uint8_t out_pin;
  volatile uint8_t* out_port;
  uint8_t in_pin;
  volatile uint8_t* in_port;

  bool enabled;
  bool trigger_pending;
  bool is_ringing;
  uint32_t ring_start_ms;

  const char* pub_topic;
  bool button_pressed;
  bool mqtt_sent;

  bool last_raw_state;
  uint32_t last_debounce_ms;
};

// Definition der beiden Klingel-Module
static ChimeState chime1 = {(1 << PORTB0), &PORTB, (1 << PORTD2), &PIND, true, false, false, 0,
                            nullptr,       false,  false,         true,  0};
static ChimeState chime2 = {(1 << PORTB1), &PORTB, (1 << PORTD3), &PIND, true, false, false, 0,
                            nullptr,       false,  false,         true,  0};

static serial::UART* g_uart = nullptr;
static serial::SPI* g_spi = nullptr;
static MQTT::MinimalMQTT* g_mqtt_client = nullptr;
static Config::LightweightConfig* g_config = nullptr;
static Ethernet::W5500Interface* g_w5500 = nullptr;

char cmd_buffer[64];
uint8_t cmd_index = 0;

extern "C" {
void disable_wdt_early(void) __attribute__((naked)) __attribute__((section(".init3")));
void disable_wdt_early(void) {
  MCUSR = 0;
  wdt_disable();
}
}

namespace {

void print_log_ptr(serial::Interface* uart_ptr, const char* progmem_text) {
  if (!uart_ptr || !progmem_text)
    return;
  char ch;
  while ((ch = pgm_read_byte(progmem_text++)) != '\0') {
    uart_ptr->send(static_cast<uint8_t>(ch));
  }
}

// Setzt den Zustand der State Machine sauber auf den aktuellen physikalischen Ist-Wert
void init_chime(ChimeState& chime) {
  chime.last_raw_state = (*chime.in_port & chime.in_pin) != 0;
  chime.last_debounce_ms = System::TimerService::millis();
  chime.button_pressed = false;
  chime.mqtt_sent = false;
  chime.is_ringing = false;
  chime.trigger_pending = false;
}

void on_mqtt_message_received(const char* topic, const uint8_t* payload, uint16_t length) {
  print_log_ptr(g_uart, PSTR("[MQTT] CMD RX on: "));
  g_uart->send_string(topic);
  print_log_ptr(g_uart, PSTR("Payload: "));
  for (uint16_t i = 0; i < length; i++) {
    g_uart->send(payload[i]);
  }
  print_log_ptr(g_uart, PSTR("\r\n"));

  ChimeState* target_chime = nullptr;
  uint16_t t_len = strlen(topic);

  if (t_len >= 2 && topic[t_len - 2] == '/') {
    if (topic[t_len - 1] == '1') {
      target_chime = &chime1;
    } else if (topic[t_len - 1] == '2') {
      target_chime = &chime2;
    }
  }

  if (!target_chime)
    return;

  if (length >= 2 && strncmp(reinterpret_cast<const char*>(payload), "ON", 2) == 0) {
    target_chime->enabled = true;
    print_log_ptr(g_uart, PSTR("[BELL] Chime enabled\r\n"));
  } else if (length >= 3 && strncmp(reinterpret_cast<const char*>(payload), "OFF", 3) == 0) {
    target_chime->enabled = false;
    print_log_ptr(g_uart, PSTR("[BELL] Chime disabled\r\n"));
  } else if (length >= 4 && strncmp(reinterpret_cast<const char*>(payload), "RING", 4) == 0) {
    target_chime->trigger_pending = true;
    print_log_ptr(g_uart, PSTR("[BELL] Ring triggered via MQTT\r\n"));
  }
}

void mqtt_subscribe_topics(const Config::SmartBellConfig& cfg) {
  if (!g_mqtt_client->is_connected())
    return;

  static char sub_topic[MQTT::kMaxTopicLength];

  strncpy(sub_topic, cfg.gong_base_topic, MQTT::kMaxTopicLength - 3);
  sub_topic[MQTT::kMaxTopicLength - 3] = '\0';
  strcat(sub_topic, "/1");
  g_mqtt_client->subscribe(sub_topic, on_mqtt_message_received);

  strncpy(sub_topic, cfg.gong_base_topic, MQTT::kMaxTopicLength - 3);
  sub_topic[MQTT::kMaxTopicLength - 3] = '\0';
  strcat(sub_topic, "/2");
  g_mqtt_client->subscribe(sub_topic, on_mqtt_message_received);
}

void process_chime(ChimeState& chime) {
  uint32_t now = System::TimerService::millis();

  // Diese Variablen behalten ihren Wert über alle Funktionsaufrufe hinweg
  static bool is_loop_started = false;
  static uint32_t loop_start_ms = 0;

  if (!is_loop_started) {
    loop_start_ms = now;  // Speichere die exakte Zeit, wann die Loop wirklich begann
    is_loop_started = true;
  }

  // Sperre die Verarbeitung für exakt 3 Sekunden NACH Eintritt in die Loop
  if ((now - loop_start_ms) < 3000) {
    chime.last_raw_state = (*chime.in_port & chime.in_pin) != 0;
    chime.last_debounce_ms = now;
    chime.button_pressed = false;
    chime.mqtt_sent = false;
    chime.trigger_pending = false;
    return;  // Abbruch!
  }

  // 2. Software-Polling & Debounce (Active-Low)
  bool raw_state = (*chime.in_port & chime.in_pin) != 0;
  if (raw_state != chime.last_raw_state) {
    chime.last_debounce_ms = now;
  }
  chime.last_raw_state = raw_state;

  if ((now - chime.last_debounce_ms) > 50) {
    if (!raw_state) {
      // Nur bei einer echten Flanke (Edge-Trigger) auslösen!
      if (!chime.button_pressed) {
        chime.button_pressed = true;
        chime.mqtt_sent = false;
        chime.trigger_pending = true;
      }
    } else {
      // Taster losgelassen: Sofort zurücksetzen, unabhängig vom Gong!
      chime.button_pressed = false;
    }
  }

  // 3. Physischen Ausgang schalten
  if (chime.trigger_pending) {
    chime.trigger_pending = false;
    if (chime.enabled && !chime.is_ringing) {
      chime.is_ringing = true;
      chime.ring_start_ms = now;
      *chime.out_port |= chime.out_pin;
    }
  }

  // 4. Timer für physischen Gong (1.5 Sekunden)
  if (chime.is_ringing) {
    if ((now - chime.ring_start_ms) >= 1500) {
      chime.is_ringing = false;
      *chime.out_port &= ~chime.out_pin;
      print_log_ptr(g_uart, PSTR("[BELL] Ring ended\r\n"));
    }
  }

  // 5. MQTT Event senden (Retry solange gedrückt)
  if (chime.button_pressed && !chime.mqtt_sent && g_mqtt_client->is_connected() &&
      chime.pub_topic) {
    static const char* payload = "1";
    if (g_mqtt_client->publish(chime.pub_topic, reinterpret_cast<const uint8_t*>(payload), 1)) {
      chime.mqtt_sent = true;
      print_log_ptr(g_uart, PSTR("[MQTT] Published button event\r\n"));
    }
  }
}

}  // namespace

ISR(TIMER0_COMPA_vect) { System::TimerService::on_1ms_tick(); }

void setup_GPIO() {
  // Globale Deaktivierung des Pull-up Disable Bits (Sicherheitshalber aktivieren)
  MCUCR &= ~(1 << PUD);

  DDRB |= (1 << PORTB0) | (1 << PORTB1);
  PORTB &= ~((1 << PORTB0) | (1 << PORTB1));

  DDRB |= kSPI_CS_W5500;
  PORTB |= kSPI_CS_W5500;

  DDRD |= kRESET_W5500;
  PORTD |= kRESET_W5500;

  DDRD &= ~((1 << PORTD2) | (1 << PORTD3));
  PORTD |= (1 << PORTD2) | (1 << PORTD3);  // Pull-ups ein
}

int main() {
  uint8_t reset_cause = MCUSR;
  MCUSR = 0;
  wdt_disable();

  setup_GPIO();

  serial::Serial_parameters uart_params = {serial::Communication_mode::kAsynchronous,
                                           serial::Asynchronous_mode::kNormal,
                                           serial::Baudrate::kBaud_19200,
                                           serial::StopBits::kOne,
                                           serial::DataBits::kEight,
                                           serial::Parity::kNone};
  serial::UART uart(uart_params);
  g_uart = &uart;

  print_log_ptr(g_uart, PSTR("\r\n=== Smart Bell Booting ===\r\n"));

  timer_interrupt::ctc_mode::setup_timer0_1ms();

  static Config::LightweightConfig config(uart);
  g_config = &config;
  config.load();
  const Config::SmartBellConfig& cfg = config.config();

  chime1.pub_topic = cfg.input1_topic;
  chime2.pub_topic = cfg.input2_topic;

  serial::SPI_parameters spi_params = {
      serial::SPI_mode::kMaster, serial::SPI_data_order::kMsb_first,
      serial::SPI_clock_polarity::kIdle_low, serial::SPI_clock_phase::kLeading,
      serial::SPI_clock_rate::k4mHz};
  serial::SPI spi(spi_params, kSPI_CS_W5500);
  g_spi = &spi;

  Ethernet::W5500Callbacks w5500_callbacks = {.hard_reset =
                                                  []() {
                                                    PORTD &= ~kRESET_W5500;
                                                    _delay_ms(1);
                                                    PORTD |= kRESET_W5500;
                                                    _delay_ms(10);
                                                  },
                                              .chip_select =
                                                  []() {
                                                    SPCR &= ~(1 << SPIE);
                                                    PORTB &= ~kSPI_CS_W5500;
                                                  },
                                              .chip_deselect =
                                                  []() {
                                                    PORTB |= kSPI_CS_W5500;
                                                    SPCR |= (1 << SPIE);
                                                  }};

  PORTB |= kSPI_CS_W5500;
  PORTD &= ~kRESET_W5500;
  _delay_ms(50);
  wdt_reset();
  PORTD |= kRESET_W5500;
  _delay_ms(100);
  wdt_reset();
  _delay_ms(100);

  static Ethernet::W5500Interface w5500(&spi, w5500_callbacks);
  g_w5500 = &w5500;
  w5500.init();

  Ethernet::MacAddress mac;
  Ethernet::IpAddress ip;
  Ethernet::SubnetMask subnet;
  Ethernet::GatewayAddress gateway;
  memcpy(mac.addr, cfg.mac, 6);
  memcpy(ip.addr, cfg.device_ip, 4);
  memcpy(subnet.addr, cfg.subnet, 4);
  memcpy(gateway.addr, cfg.gateway, 4);
  w5500.set_network_config(&mac, &ip, &subnet, &gateway);

  static MQTT::MinimalMQTT mqtt_client_instance{&uart};
  g_mqtt_client = &mqtt_client_instance;

  MQTT::Config mqtt_config;
  memcpy(mqtt_config.broker_ip, cfg.broker_ip, 4);
  mqtt_config.broker_port = cfg.broker_port;
  strncpy(mqtt_config.client_id, cfg.client_id, MQTT::kMaxClientIdLength);
  mqtt_config.client_id[MQTT::kMaxClientIdLength - 1] = '\0';
  mqtt_config.use_auth = false;
  mqtt_config.keepalive = 60;

  bool mqtt_configured =
      (cfg.broker_ip[0] | cfg.broker_ip[1] | cfg.broker_ip[2] | cfg.broker_ip[3]) != 0;
  if (mqtt_configured) {
    if (mqtt_client_instance.connect(mqtt_config)) {
      mqtt_subscribe_topics(cfg);
    }
  }

  print_log_ptr(g_uart, PSTR("[SYS] App Engine fully operational!\r\n\r\n"));
  wdt_enable(WDTO_4S);
  sei();

  uint32_t last_millis = 0;
  uint32_t last_mqtt_retry_ms = 0;

  init_chime(chime1);
  init_chime(chime2);

  chime1.last_raw_state = (*chime1.in_port & chime1.in_pin) != 0;
  chime2.last_raw_state = (*chime2.in_port & chime2.in_pin) != 0;

  while (1) {
    wdt_reset();
    g_mqtt_client->loop();

    if (mqtt_configured && !g_mqtt_client->is_connected()) {
      uint32_t now = System::TimerService::millis();
      if ((now - last_mqtt_retry_ms) >= 5000) {
        last_mqtt_retry_ms = now;
        const Config::SmartBellConfig& live_cfg = g_config->config();

        static MQTT::Config retry_cfg;
        memcpy(retry_cfg.broker_ip, live_cfg.broker_ip, 4);
        retry_cfg.broker_port = live_cfg.broker_port;
        strncpy(retry_cfg.client_id, live_cfg.client_id, MQTT::kMaxClientIdLength);
        retry_cfg.client_id[MQTT::kMaxClientIdLength - 1] = '\0';
        retry_cfg.use_auth = false;
        retry_cfg.keepalive = 60;

        print_log_ptr(g_uart, PSTR("[MQTT] Reconnect...\r\n"));
        if (g_mqtt_client->connect(retry_cfg)) {
          mqtt_subscribe_topics(live_cfg);
        }
      }
    }

    if (g_config->consume_save_flag()) {
      const Config::SmartBellConfig& live_cfg = g_config->config();
      bool has_broker = (live_cfg.broker_ip[0] | live_cfg.broker_ip[1] | live_cfg.broker_ip[2] |
                         live_cfg.broker_ip[3]) != 0;
      mqtt_configured = has_broker;
      if (has_broker) {
        static MQTT::Config reconnect_cfg;
        memcpy(reconnect_cfg.broker_ip, live_cfg.broker_ip, 4);
        reconnect_cfg.broker_port = live_cfg.broker_port;
        strncpy(reconnect_cfg.client_id, live_cfg.client_id, MQTT::kMaxClientIdLength);
        reconnect_cfg.client_id[MQTT::kMaxClientIdLength - 1] = '\0';
        reconnect_cfg.use_auth = false;
        reconnect_cfg.keepalive = 60;
        if (g_mqtt_client->connect(reconnect_cfg)) {
          mqtt_subscribe_topics(live_cfg);
        }
      }
    }

    // State Machines ausführen
    process_chime(chime1);
    process_chime(chime2);

    // UART Parser
    while (uart.is_read_data_available()) {
      char c = uart.read_byte();
      if (c == '\r' || c == '\n') {
        if (cmd_index > 0) {
          cmd_buffer[cmd_index] = '\0';
          print_log_ptr(g_uart, PSTR("\r\n"));
          g_config->process_command(cmd_buffer);
          cmd_index = 0;
        }
      } else if (c == '\b' || c == 0x7F) {
        if (cmd_index > 0) {
          cmd_index--;
          print_log_ptr(g_uart, PSTR("\b \b"));
        }
      } else if (cmd_index < 31) {
        cmd_buffer[cmd_index++] = c;
        char echo[2] = {c, '\0'};
        uart.send_string(echo);
      }
    }

    uint32_t current_millis = System::TimerService::millis();
    if (current_millis - last_millis > 100) {
      last_millis = current_millis;
      _delay_ms(10);
    }
  }
  return 0;
}