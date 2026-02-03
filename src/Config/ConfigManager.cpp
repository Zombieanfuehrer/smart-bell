#include "Config/ConfigManager.h"

#ifdef __AVR__
#include <avr/eeprom.h>
#endif

#include <string.h>
#include <stdlib.h>

namespace Config {

// EEPROM address for configuration
constexpr uint16_t EEPROM_CONFIG_ADDR = 0;
constexpr uint32_t CONFIG_MAGIC = 0xBEEFCAFE;

ConfigManager::ConfigManager(serial::UART* uart)
    : uart_(uart) {
  load_defaults();
}

void ConfigManager::load_defaults() {
  // MQTT defaults
  strncpy(config_.mqtt_broker, "192.168.1.100", 63);
  config_.mqtt_broker[63] = '\0';
  config_.mqtt_port = 1883;
  strncpy(config_.mqtt_client_id, "smart-bell", 31);
  config_.mqtt_client_id[31] = '\0';
  config_.mqtt_username[0] = '\0'; // Empty by default
  config_.mqtt_password[0] = '\0'; // Empty by default
  strncpy(config_.mqtt_topic_ring, "home/bell/ring", 47);
  config_.mqtt_topic_ring[47] = '\0';
  strncpy(config_.mqtt_topic_control, "home/bell/control", 47);
  config_.mqtt_topic_control[47] = '\0';
  config_.mqtt_keepalive = 60;
  
  // Network defaults (DHCP enabled)
  config_.use_dhcp = true;
  config_.static_ip[0] = 192;
  config_.static_ip[1] = 168;
  config_.static_ip[2] = 1;
  config_.static_ip[3] = 177;
  config_.subnet_mask[0] = 255;
  config_.subnet_mask[1] = 255;
  config_.subnet_mask[2] = 255;
  config_.subnet_mask[3] = 0;
  config_.gateway[0] = 192;
  config_.gateway[1] = 168;
  config_.gateway[2] = 1;
  config_.gateway[3] = 1;
  config_.mac_address[0] = 0x00;
  config_.mac_address[1] = 0x08;
  config_.mac_address[2] = 0xDC;
  config_.mac_address[3] = 0xAB;
  config_.mac_address[4] = 0xCD;
  config_.mac_address[5] = 0xEF;
  
  // Bell settings
  config_.ring_duration = 6; // 6 seconds
  config_.debounce_delay = 50; // 50 ms
  
  // System settings
  config_.reconnect_interval = 30; // 30 seconds
  
  config_.magic_number = CONFIG_MAGIC;
  
  if (uart_) {
    uart_->send_string("[Config] Loaded default configuration\r\n");
  }
}

bool ConfigManager::save_to_eeprom() {
#ifdef __AVR__
  config_.magic_number = CONFIG_MAGIC;
  eeprom_update_block(&config_, (void*)EEPROM_CONFIG_ADDR, sizeof(SmartBellConfig));
  
  if (uart_) {
    uart_->send_string("[Config] Configuration saved to EEPROM\r\n");
  }
  return true;
#else
  if (uart_) {
    uart_->send_string("[Config] EEPROM not available on this platform\r\n");
  }
  return false;
#endif
}

bool ConfigManager::load_from_eeprom() {
#ifdef __AVR__
  SmartBellConfig temp_config;
  eeprom_read_block(&temp_config, (void*)EEPROM_CONFIG_ADDR, sizeof(SmartBellConfig));
  
  if (temp_config.magic_number == CONFIG_MAGIC) {
    memcpy(&config_, &temp_config, sizeof(SmartBellConfig));
    if (uart_) {
      uart_->send_string("[Config] Configuration loaded from EEPROM\r\n");
    }
    return true;
  } else {
    if (uart_) {
      uart_->send_string("[Config] No valid configuration in EEPROM, using defaults\r\n");
    }
    load_defaults();
    return false;
  }
#else
  if (uart_) {
    uart_->send_string("[Config] EEPROM not available, using defaults\r\n");
  }
  load_defaults();
  return false;
#endif
}

void ConfigManager::get_mqtt_config(MQTT::MQTTConfig& mqtt_config) const {
  strncpy(mqtt_config.broker_hostname, config_.mqtt_broker, 63);
  mqtt_config.broker_hostname[63] = '\0';
  mqtt_config.broker_port = config_.mqtt_port;
  strncpy(mqtt_config.client_id, config_.mqtt_client_id, 31);
  mqtt_config.client_id[31] = '\0';
  strncpy(mqtt_config.username, config_.mqtt_username, 31);
  mqtt_config.username[31] = '\0';
  strncpy(mqtt_config.password, config_.mqtt_password, 31);
  mqtt_config.password[31] = '\0';
  mqtt_config.keepalive = config_.mqtt_keepalive;
  mqtt_config.reconnect_interval = config_.reconnect_interval;
}

void ConfigManager::set_mqtt_broker(const char* broker, uint16_t port) {
  strncpy(config_.mqtt_broker, broker, 63);
  config_.mqtt_broker[63] = '\0';
  config_.mqtt_port = port;
}

void ConfigManager::set_mqtt_credentials(const char* client_id, const char* username, const char* password) {
  strncpy(config_.mqtt_client_id, client_id, 31);
  config_.mqtt_client_id[31] = '\0';
  strncpy(config_.mqtt_username, username, 31);
  config_.mqtt_username[31] = '\0';
  strncpy(config_.mqtt_password, password, 31);
  config_.mqtt_password[31] = '\0';
}

void ConfigManager::set_mqtt_topics(const char* topic_ring, const char* topic_control) {
  strncpy(config_.mqtt_topic_ring, topic_ring, 47);
  config_.mqtt_topic_ring[47] = '\0';
  strncpy(config_.mqtt_topic_control, topic_control, 47);
  config_.mqtt_topic_control[47] = '\0';
}

void ConfigManager::set_dhcp(bool enable) {
  config_.use_dhcp = enable;
}

void ConfigManager::set_static_ip(const uint8_t* ip, const uint8_t* subnet, const uint8_t* gateway) {
  memcpy(config_.static_ip, ip, 4);
  memcpy(config_.subnet_mask, subnet, 4);
  memcpy(config_.gateway, gateway, 4);
}

void ConfigManager::print_config() const {
  if (!uart_) return;
  
  uart_->send_string("\r\n=== Smart Bell Configuration ===\r\n");
  uart_->send_string("MQTT Broker: ");
  uart_->send_string(config_.mqtt_broker);
  uart_->send_string(":");
  char port_str[8];
  itoa(config_.mqtt_port, port_str, 10);
  uart_->send_string(port_str);
  uart_->send_string("\r\n");
  
  uart_->send_string("Client ID: ");
  uart_->send_string(config_.mqtt_client_id);
  uart_->send_string("\r\n");
  
  uart_->send_string("Username: ");
  uart_->send_string(config_.mqtt_username[0] ? config_.mqtt_username : "(none)");
  uart_->send_string("\r\n");
  
  uart_->send_string("Password: ");
  uart_->send_string(config_.mqtt_password[0] ? "********" : "(none)");
  uart_->send_string("\r\n");
  
  uart_->send_string("Ring Topic: ");
  uart_->send_string(config_.mqtt_topic_ring);
  uart_->send_string("\r\n");
  
  uart_->send_string("Control Topic: ");
  uart_->send_string(config_.mqtt_topic_control);
  uart_->send_string("\r\n");
  
  uart_->send_string("DHCP: ");
  uart_->send_string(config_.use_dhcp ? "enabled" : "disabled");
  uart_->send_string("\r\n");
  
  if (!config_.use_dhcp) {
    uart_->send_string("Static IP: ");
    char ip_str[16];
    snprintf(ip_str, 16, "%d.%d.%d.%d", config_.static_ip[0], config_.static_ip[1], 
             config_.static_ip[2], config_.static_ip[3]);
    uart_->send_string(ip_str);
    uart_->send_string("\r\n");
  }
  
  uart_->send_string("Ring Duration: ");
  char dur_str[8];
  itoa(config_.ring_duration, dur_str, 10);
  uart_->send_string(dur_str);
  uart_->send_string(" seconds\r\n");
  
  uart_->send_string("Reconnect Interval: ");
  char recon_str[8];
  itoa(config_.reconnect_interval, recon_str, 10);
  uart_->send_string(recon_str);
  uart_->send_string(" seconds\r\n");
  
  uart_->send_string("===============================\r\n\r\n");
}

bool ConfigManager::process_uart_command(const char* command) {
  if (starts_with(command, "config show")) {
    print_config();
    return true;
  }
  else if (starts_with(command, "config save")) {
    return save_to_eeprom();
  }
  else if (starts_with(command, "config load")) {
    return load_from_eeprom();
  }
  else if (starts_with(command, "config defaults")) {
    load_defaults();
    uart_->send_string("[Config] Defaults loaded\r\n");
    return true;
  }
  else if (starts_with(command, "set broker ")) {
    return parse_set_broker(command + 11);
  }
  else if (starts_with(command, "set cred ")) {
    return parse_set_credentials(command + 9);
  }
  else if (starts_with(command, "set topics ")) {
    return parse_set_topics(command + 11);
  }
  else if (starts_with(command, "set dhcp ")) {
    return parse_set_dhcp(command + 9);
  }
  else if (starts_with(command, "set ip ")) {
    return parse_set_ip(command + 7);
  }
  else if (starts_with(command, "help")) {
    uart_->send_string("\r\nAvailable commands:\r\n");
    uart_->send_string("  config show - Show current configuration\r\n");
    uart_->send_string("  config save - Save configuration to EEPROM\r\n");
    uart_->send_string("  config load - Load configuration from EEPROM\r\n");
    uart_->send_string("  config defaults - Load default configuration\r\n");
    uart_->send_string("  set broker <host> <port> - Set MQTT broker\r\n");
    uart_->send_string("  set cred <id> <user> <pass> - Set MQTT credentials\r\n");
    uart_->send_string("  set topics <ring> <control> - Set MQTT topics\r\n");
    uart_->send_string("  set dhcp <on|off> - Enable/disable DHCP\r\n");
    uart_->send_string("  set ip <ip> <subnet> <gateway> - Set static IP\r\n");
    uart_->send_string("  help - Show this help\r\n\r\n");
    return true;
  }
  
  return false;
}

// Private helper methods

bool ConfigManager::parse_set_broker(const char* args) {
  char broker[64];
  uint16_t port;
  
  // Parse: <host> <port>
  int parsed = sscanf(args, "%63s %hu", broker, &port);
  if (parsed == 2) {
    set_mqtt_broker(broker, port);
    uart_->send_string("[Config] Broker set to: ");
    uart_->send_string(broker);
    uart_->send_string("\r\n");
    return true;
  }
  
  uart_->send_string("[Config] Usage: set broker <host> <port>\r\n");
  return false;
}

bool ConfigManager::parse_set_credentials(const char* args) {
  char client_id[32], username[32], password[32];
  
  // Parse: <client_id> <username> <password>
  int parsed = sscanf(args, "%31s %31s %31s", client_id, username, password);
  if (parsed == 3) {
    set_mqtt_credentials(client_id, username, password);
    uart_->send_string("[Config] Credentials updated\r\n");
    return true;
  }
  
  uart_->send_string("[Config] Usage: set cred <client_id> <username> <password>\r\n");
  return false;
}

bool ConfigManager::parse_set_topics(const char* args) {
  char topic_ring[48], topic_control[48];
  
  // Parse: <ring_topic> <control_topic>
  int parsed = sscanf(args, "%47s %47s", topic_ring, topic_control);
  if (parsed == 2) {
    set_mqtt_topics(topic_ring, topic_control);
    uart_->send_string("[Config] Topics updated\r\n");
    return true;
  }
  
  uart_->send_string("[Config] Usage: set topics <ring_topic> <control_topic>\r\n");
  return false;
}

bool ConfigManager::parse_set_dhcp(const char* args) {
  if (starts_with(args, "on")) {
    set_dhcp(true);
    uart_->send_string("[Config] DHCP enabled\r\n");
    return true;
  } else if (starts_with(args, "off")) {
    set_dhcp(false);
    uart_->send_string("[Config] DHCP disabled\r\n");
    return true;
  }
  
  uart_->send_string("[Config] Usage: set dhcp <on|off>\r\n");
  return false;
}

bool ConfigManager::parse_set_ip(const char* args) {
  uint8_t ip[4], subnet[4], gateway[4];
  
  // Parse: xxx.xxx.xxx.xxx xxx.xxx.xxx.xxx xxx.xxx.xxx.xxx
  int parsed = sscanf(args, "%hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu",
                      &ip[0], &ip[1], &ip[2], &ip[3],
                      &subnet[0], &subnet[1], &subnet[2], &subnet[3],
                      &gateway[0], &gateway[1], &gateway[2], &gateway[3]);
  
  if (parsed == 12) {
    set_static_ip(ip, subnet, gateway);
    uart_->send_string("[Config] Static IP configured\r\n");
    return true;
  }
  
  uart_->send_string("[Config] Usage: set ip <ip> <subnet> <gateway>\r\n");
  uart_->send_string("[Config] Example: set ip 192.168.1.100 255.255.255.0 192.168.1.1\r\n");
  return false;
}

void ConfigManager::parse_ip_address(const char* str, uint8_t* ip) {
  sscanf(str, "%hhu.%hhu.%hhu.%hhu", &ip[0], &ip[1], &ip[2], &ip[3]);
}

bool ConfigManager::starts_with(const char* str, const char* prefix) {
  return strncmp(str, prefix, strlen(prefix)) == 0;
}

} // namespace Config
