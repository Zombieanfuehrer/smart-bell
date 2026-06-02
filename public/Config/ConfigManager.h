#ifndef PUBLIC_CONFIG_CONFIGMANAGER_H_
#define PUBLIC_CONFIG_CONFIGMANAGER_H_

#include <stdint.h>
#include "Serial/UART.h"

namespace Config {

// Configuration storage structure
struct SmartBellConfig {
  // MQTT settings
  uint8_t mqtt_broker_ip[4];  // Broker IP address (not hostname - saves RAM!)
  uint16_t mqtt_port;
  char mqtt_client_id[24];
  char mqtt_username[24];
  char mqtt_password[24];
  char mqtt_topic_ring[32];
  char mqtt_topic_control[32];
  uint16_t mqtt_keepalive;

  // Network settings
  bool use_dhcp;
  uint8_t static_ip[4];
  uint8_t subnet_mask[4];
  uint8_t gateway[4];
  uint8_t mac_address[6];

  // Bell settings
  uint8_t ring_duration;    // seconds
  uint16_t debounce_delay;  // milliseconds

  // System settings
  uint16_t reconnect_interval;  // seconds

  // Configuration validity marker
  uint32_t magic_number;  // 0xBEEFCAFE if valid
};

class ConfigManager {
 public:
  ConfigManager(serial::UART* uart);
  ~ConfigManager() = default;

  // Load/Save configuration
  void load_defaults();
  bool save_to_eeprom();
  bool load_from_eeprom();

  // Getters
  const SmartBellConfig& get_config() const { return config_; }
  // Note: get_mqtt_config() removed - use get_config() and map fields directly

  // Setters
  void set_mqtt_broker(const uint8_t* broker_ip, uint16_t port);
  void set_mqtt_credentials(const char* client_id, const char* username, const char* password);
  void set_mqtt_topics(const char* topic_ring, const char* topic_control);
  void set_dhcp(bool enable);
  void set_static_ip(const uint8_t* ip, const uint8_t* subnet, const uint8_t* gateway);

  // Configuration via UART
  void print_config() const;
  bool process_uart_command(const char* command);

 private:
  SmartBellConfig config_;
  serial::UART* uart_;

  // Command parsing helpers
  bool parse_set_broker(const char* args);
  bool parse_set_credentials(const char* args);
  bool parse_set_topics(const char* args);
  bool parse_set_dhcp(const char* args);
  bool parse_set_ip(const char* args);

  // Utility functions
  void parse_ip_address(const char* str, uint8_t* ip);
  bool starts_with(const char* str, const char* prefix);
};

}  // namespace Config

#endif  // PUBLIC_CONFIG_CONFIGMANAGER_H_
