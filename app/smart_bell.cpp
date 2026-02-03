#ifdef __AVR__
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#endif

#include "SetupWDT.h"
#include "SetupEXT_IN_Interrupt.h"
#include "SetupTimer.h"
#include "Serial/UART.h"
#include "Serial/SPI.h"
#include "Ethernet/W5500/W5500Interface.h"
#include "Ethernet/EmbeddedSocketInterface.h"
#include "MQTT/MQTTClient.h"
#include "Config/ConfigManager.h"

#include <string.h>

// Global state variables
volatile uint8_t timer_counter = 0;
volatile bool button_pressed = false;
volatile bool ring_active = false;

bool bell_enabled = true;
bool mqtt_ring_sent = false;
char command_buffer[128];
uint8_t command_index = 0;

// Hardware pins
static const constexpr uint8_t kSPI_CS_W5500 = (1 << PORTB2);
static const constexpr uint8_t kRESET_W5500 = (1 << PORTD4);
static const constexpr uint8_t kRING_OUTPUT = (1 << PORTB0);

// Timing constants
constexpr const uint16_t kTimer_compare_value = 62499; // For 1Hz with prescaler 64

// Button interrupt (INT0 on PD2)
ISR(INT0_vect) {
  if (!button_pressed && bell_enabled) {
    button_pressed = true;
    ring_active = true;
    timer_counter = 0;
    PORTB |= kRING_OUTPUT;  // Activate ring output
  }
}

// Timer interrupt for ring duration
ISR(TIMER1_COMPA_vect) {
  timer_counter++;
}

// Setup GPIO pins
void setup_GPIO() {
  // Ring output pin
  DDRB |= kRING_OUTPUT;  // Set as output
  PORTB &= ~kRING_OUTPUT; // Ensure low initially
  
  // SPI CS pin
  DDRB |= kSPI_CS_W5500;  // Set as output
  PORTB |= kSPI_CS_W5500;  // Set high (inactive)
  
  // W5500 Reset pin
  DDRD |= kRESET_W5500;  // Set as output
  PORTD |= kRESET_W5500;  // Set high
}

// MQTT message callback for control topic
void mqtt_control_callback(const char* topic, const uint8_t* payload, uint16_t length) {
  // Create a null-terminated string from payload
  char message[64];
  if (length > 63) length = 63;
  memcpy(message, payload, length);
  message[length] = '\0';
  
  // Parse commands
  if (strcmp(message, "disable") == 0) {
    bell_enabled = false;
  } else if (strcmp(message, "enable") == 0) {
    bell_enabled = true;
  } else if (strcmp(message, "status") == 0) {
    // Status request - will be handled in main loop
  }
}

int main() {
  wdt_disable();  // Disable watchdog initially
  
  // Setup hardware
  setup_GPIO();
  
  // Configure UART
  serial::Serial_parameters uart_params = {
    serial::Communication_mode::kAsynchronous,
    serial::Asynchronous_mode::kNormal,
    serial::Baudrate::kBaud_19200,
    serial::StopBits::kOne,
    serial::DataBits::kEight,
    serial::Parity::kNone
  };
  serial::UART uart(uart_params);
  
  uart.send_string("\r\n");
  uart.send_string("=====================================\r\n");
  uart.send_string(" Smart Bell with MQTT - Version 1.0\r\n");
  uart.send_string("=====================================\r\n\r\n");
  
  // Initialize configuration
  Config::ConfigManager config_manager(&uart);
  
  // Try to load config from EEPROM, fallback to defaults
  if (!config_manager.load_from_eeprom()) {
    uart.send_string("[System] Using default configuration\r\n");
  }
  
  // Show configuration
  config_manager.print_config();
  uart.send_string("[System] Type 'help' for available commands\r\n\r\n");
  
  // Configure SPI
  serial::SPI_parameters spi_params = {
    serial::SPI_mode::kMaster,
    serial::SPI_data_order::kMsb_first,
    serial::SPI_clock_polarity::kIdle_low,
    serial::SPI_clock_phase::kLeading,
    serial::SPI_clock_rate::k4mHz
  };
  serial::SPI spi(spi_params, kSPI_CS_W5500);
  
  // Configure W5500 callbacks
  Ethernet::W5500Callbacks w5500_callbacks = {
    .hard_reset = []() {
      PORTD &= ~kRESET_W5500;  // Reset active (low)
      _delay_ms(1);
      PORTD |= kRESET_W5500;   // Reset inactive (high)
      _delay_ms(10);
    },
    .chip_select = []() {
      PORTB &= ~kSPI_CS_W5500;  // CS active (low)
    },
    .chip_deselect = []() {
      PORTB |= kSPI_CS_W5500;   // CS inactive (high)
    }
  };
  
  // Initialize W5500
  uart.send_string("[Ethernet] Initializing W5500...\r\n");
  Ethernet::W5500Interface w5500(&spi, w5500_callbacks, &uart);
  w5500.init();
  uart.send_string("[Ethernet] W5500 initialized\r\n");
  
  // Configure network
  const Config::SmartBellConfig& config = config_manager.get_config();
  Ethernet::MacAddress mac;
  memcpy(mac.addr, config.mac_address, 6);
  
  if (config.use_dhcp) {
    uart.send_string("[Ethernet] DHCP not yet implemented, using static IP\r\n");
  }
  
  // Use static IP configuration
  Ethernet::IpAddress ip;
  Ethernet::SubnetMask subnet;
  Ethernet::GatewayAddress gateway;
  
  memcpy(ip.addr, config.static_ip, 4);
  memcpy(subnet.addr, config.subnet_mask, 4);
  memcpy(gateway.addr, config.gateway, 4);
  
  uart.send_string("[Ethernet] Configuring network...\r\n");
  w5500.set_network_config(&mac, &ip, &subnet, &gateway);
  uart.send_string("[Ethernet] Network configured\r\n");
  
  // Initialize MQTT
  Ethernet::EmbeddedSocketW5500 socket_manager(&w5500);
  MQTT::MQTTClient mqtt_client(&socket_manager, &uart);
  
  // Get MQTT configuration
  MQTT::MQTTConfig mqtt_config;
  config_manager.get_mqtt_config(mqtt_config);
  
  // Connect to MQTT broker
  uart.send_string("[MQTT] Connecting to broker...\r\n");
  if (mqtt_client.connect(mqtt_config)) {
    uart.send_string("[MQTT] Connected to broker\r\n");
    
    // Subscribe to control topic
    const char* control_topic = config.mqtt_topic_control;
    mqtt_client.subscribe(control_topic, MQTT::MQTTQoS::QoS0, mqtt_control_callback);
  } else {
    uart.send_string("[MQTT] Failed to connect to broker\r\n");
  }
  
  // Setup interrupts
  external_pin_interrupt::setup_INT0_PullUpResistorFallingEdge();
  timer_interrupt::ctc_mode::setup_timer0_ctc_mode(timer_interrupt::Prescaler::DIV64, kTimer_compare_value);
  
  // Configure watchdog (4 seconds timeout)
  configure_wdt::timeout_4sec_reset_power::setup_WTD();
  
  // Enable global interrupts
  sei();
  
  uart.send_string("[System] Smart Bell started\r\n\r\n");
  
  // Main loop
  while (1) {
    wdt_reset();  // Reset watchdog
    
    // Process UART commands
    if (uart.is_read_data_available()) {
      char received = uart.read_byte();
      
      if (received == '\r' || received == '\n') {
        if (command_index > 0) {
          command_buffer[command_index] = '\0';
          
          // Echo newline
          uart.send_string("\r\n");
          
          // Process command
          if (!config_manager.process_uart_command(command_buffer)) {
            uart.send_string("[System] Unknown command. Type 'help' for available commands.\r\n");
          }
          
          command_index = 0;
        }
      } else if (received == 8 || received == 127) { // Backspace
        if (command_index > 0) {
          command_index--;
          uart.send_string("\b \b"); // Erase character
        }
      } else if (command_index < sizeof(command_buffer) - 1) {
        command_buffer[command_index++] = received;
        uart.send(received); // Echo
      }
    }
    
    // Handle button press
    if (button_pressed && !mqtt_ring_sent) {
      if (mqtt_client.is_connected()) {
        const char* ring_topic = config.mqtt_topic_ring;
        const char* payload = "RING";
        
        uart.send_string("[Bell] Button pressed - publishing MQTT message\r\n");
        if (mqtt_client.publish(ring_topic, payload, MQTT::MQTTQoS::QoS0, false)) {
          mqtt_ring_sent = true;
        }
      } else {
        uart.send_string("[Bell] Button pressed but MQTT not connected\r\n");
        mqtt_ring_sent = true; // Don't retry immediately
      }
    }
    
    // Handle ring duration
    if (ring_active && timer_counter >= config.ring_duration) {
      ring_active = false;
      button_pressed = false;
      mqtt_ring_sent = false;
      PORTB &= ~kRING_OUTPUT;  // Deactivate ring output
      timer_counter = 0;
      uart.send_string("[Bell] Ring completed\r\n");
    }
    
    // Process MQTT (keepalive, incoming messages, reconnect)
    mqtt_client.loop();
    
    // Small delay to prevent busy-waiting
    _delay_ms(10);
  }
  
  return 0;
}
