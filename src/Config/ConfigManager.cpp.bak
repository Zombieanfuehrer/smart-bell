#include "Config/ConfigManager.h"
#include <string.h>
#include "SetupWDT.h"

#ifdef __AVR__
#include <avr/eeprom.h>
#endif

namespace Config {

// EEPROM address for stored config (start at address 0)
#ifdef __AVR__
static StoredConfig EEMEM eeprom_config;
#endif

ConfigManager::ConfigManager() { reset_to_defaults(); }

bool ConfigManager::load() {
#ifdef __AVR__
  StoredConfig stored;

  // Pause WDT during EEPROM read
  configure_wdt::pause();

  eeprom_read_block(&stored, &eeprom_config, sizeof(StoredConfig));

  configure_wdt::resume();

  if (!validate_config(stored)) {
    config_.is_valid = false;
    return false;
  }

  // Copy to runtime config
  safe_strcpy(config_.broker, stored.broker, kMaxBrokerLength);
  config_.broker_port = stored.broker_port;
  safe_strcpy(config_.username, stored.username, kMaxUsernameLength);
  safe_strcpy(config_.password, stored.password, kMaxPasswordLength);
  safe_strcpy(config_.client_id, stored.client_id, kMaxClientIdLength);

  // Determine if broker is hostname or IP
  config_.broker_is_hostname = contains_alpha(config_.broker);

  // Parse IP if not a hostname
  if (!config_.broker_is_hostname) {
    parse_ip_address(config_.broker, config_.broker_ip);
  }

  config_.is_valid = true;
  return true;
#else
  // For testing on x86, always return false (no EEPROM)
  return false;
#endif
}

bool ConfigManager::save() {
#ifdef __AVR__
  StoredConfig stored;

  // Set magic bytes
  stored.magic[0] = kConfigMagic0;
  stored.magic[1] = kConfigMagic1;

  // Copy current config
  safe_strcpy(stored.broker, config_.broker, kMaxBrokerLength);
  stored.broker_port = config_.broker_port;
  safe_strcpy(stored.username, config_.username, kMaxUsernameLength);
  safe_strcpy(stored.password, config_.password, kMaxPasswordLength);
  safe_strcpy(stored.client_id, config_.client_id, kMaxClientIdLength);

  // Calculate and set checksum
  stored.checksum = calculate_checksum(stored);

  // Pause WDT during EEPROM write
  configure_wdt::pause();

  eeprom_update_block(&stored, &eeprom_config, sizeof(StoredConfig));

  configure_wdt::resume();

  // Update runtime config validity
  config_.broker_is_hostname = contains_alpha(config_.broker);
  if (!config_.broker_is_hostname) {
    parse_ip_address(config_.broker, config_.broker_ip);
  }
  config_.is_valid = true;

  return true;
#else
  // For testing, just mark as valid
  config_.is_valid = true;
  return true;
#endif
}

void ConfigManager::reset_to_defaults() {
  memset(&config_, 0, sizeof(RuntimeConfig));
  config_.broker[0] = '\0';
  config_.broker_port = kDefaultBrokerPort;
  config_.username[0] = '\0';
  config_.password[0] = '\0';
  safe_strcpy(config_.client_id, kDefaultClientId, kMaxClientIdLength);
  config_.broker_is_hostname = false;
  config_.is_valid = false;
}

bool ConfigManager::is_valid() const { return config_.is_valid && config_.broker[0] != '\0'; }

bool ConfigManager::has_stored_config() const {
#ifdef __AVR__
  uint8_t magic0 = eeprom_read_byte((const uint8_t*)&eeprom_config.magic[0]);
  uint8_t magic1 = eeprom_read_byte((const uint8_t*)&eeprom_config.magic[1]);
  return (magic0 == kConfigMagic0 && magic1 == kConfigMagic1);
#else
  return false;
#endif
}

void ConfigManager::set_broker(const char* broker) {
  safe_strcpy(config_.broker, broker, kMaxBrokerLength);
  config_.broker_is_hostname = contains_alpha(broker);
  if (!config_.broker_is_hostname) {
    parse_ip_address(config_.broker, config_.broker_ip);
  }
}

void ConfigManager::set_port(uint16_t port) { config_.broker_port = port; }

void ConfigManager::set_username(const char* username) {
  safe_strcpy(config_.username, username, kMaxUsernameLength);
}

void ConfigManager::set_password(const char* password) {
  safe_strcpy(config_.password, password, kMaxPasswordLength);
}

void ConfigManager::set_client_id(const char* client_id) {
  safe_strcpy(config_.client_id, client_id, kMaxClientIdLength);
}

const char* ConfigManager::get_broker() const { return config_.broker; }

uint16_t ConfigManager::get_port() const { return config_.broker_port; }

const char* ConfigManager::get_username() const { return config_.username; }

const char* ConfigManager::get_password() const { return config_.password; }

const char* ConfigManager::get_client_id() const { return config_.client_id; }

bool ConfigManager::broker_is_hostname() const { return config_.broker_is_hostname; }

bool ConfigManager::get_broker_ip(uint8_t* ip_out) const {
  if (config_.broker_is_hostname) {
    return false;
  }
  for (int i = 0; i < 4; i++) {
    ip_out[i] = config_.broker_ip[i];
  }
  return true;
}

const RuntimeConfig& ConfigManager::get_runtime_config() const { return config_; }

uint8_t ConfigManager::calculate_checksum(const StoredConfig& config) const {
  uint8_t checksum = 0;
  const uint8_t* data = reinterpret_cast<const uint8_t*>(&config);
  // XOR all bytes except the checksum field itself
  for (size_t i = 0; i < sizeof(StoredConfig) - 1; i++) {
    checksum ^= data[i];
  }
  return checksum;
}

bool ConfigManager::validate_config(const StoredConfig& config) const {
  // Check magic bytes
  if (config.magic[0] != kConfigMagic0 || config.magic[1] != kConfigMagic1) {
    return false;
  }
  // Verify checksum
  uint8_t expected = calculate_checksum(config);
  return config.checksum == expected;
}

bool ConfigManager::contains_alpha(const char* str) const {
  for (size_t i = 0; str[i] != '\0'; i++) {
    if ((str[i] >= 'a' && str[i] <= 'z') || (str[i] >= 'A' && str[i] <= 'Z')) {
      return true;
    }
  }
  return false;
}

bool ConfigManager::parse_ip_address(const char* str, uint8_t* ip_out) const {
  uint8_t octet = 0;
  uint8_t octet_index = 0;
  uint8_t digit_count = 0;

  for (size_t i = 0; str[i] != '\0' && octet_index < 4; i++) {
    if (str[i] >= '0' && str[i] <= '9') {
      octet = octet * 10 + (str[i] - '0');
      digit_count++;
      if (digit_count > 3 || octet > 255) {
        return false;  // Invalid octet
      }
    } else if (str[i] == '.') {
      if (digit_count == 0) {
        return false;  // Empty octet
      }
      ip_out[octet_index++] = octet;
      octet = 0;
      digit_count = 0;
    } else {
      return false;  // Invalid character
    }
  }

  // Handle last octet
  if (digit_count > 0 && octet_index == 3) {
    ip_out[octet_index] = octet;
    return true;
  }

  return false;
}

void ConfigManager::safe_strcpy(char* dest, const char* src, uint8_t max_len) {
  if (max_len == 0)
    return;

  size_t i = 0;
  while (i < static_cast<size_t>(max_len - 1) && src[i] != '\0') {
    dest[i] = src[i];
    i++;
  }
  dest[i] = '\0';
}

}  // namespace Config
