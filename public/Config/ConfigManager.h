#ifndef PUBLIC_CONFIG_CONFIGMANAGER_H_
#define PUBLIC_CONFIG_CONFIGMANAGER_H_

#include <stdbool.h>
#include <stdint.h>

namespace Config {

// Magic bytes to validate EEPROM content
static constexpr uint8_t kConfigMagic0 = 0xBE;
static constexpr uint8_t kConfigMagic1 = 0x11;

// Default values
static constexpr uint16_t kDefaultBrokerPort = 1883;
static constexpr const char* kDefaultClientId = "smartbell";

// Field size limits
static constexpr uint8_t kMaxUsernameLength = 32;
static constexpr uint8_t kMaxPasswordLength = 32;
static constexpr uint8_t kMaxClientIdLength = 16;

/**
 * @brief Configuration structure stored in EEPROM.
 * Static IP mode - no DHCP/DNS, all addresses stored directly.
 * Total size: ~100 bytes (fits in ATmega328P's 1KB EEPROM)
 */
struct StoredConfig {
  uint8_t magic[2];                    // 0xBE, 0x11 for validation
  uint8_t device_ip[4];                // Static IP for this device
  uint8_t subnet[4];                   // Subnet mask (e.g. 255.255.255.0)
  uint8_t gateway[4];                  // Gateway IP
  uint8_t broker_ip[4];                // MQTT broker IP address
  uint16_t broker_port;                // Default: 1883
  char username[kMaxUsernameLength];   // Null-terminated, plaintext
  char password[kMaxPasswordLength];   // Null-terminated, plaintext
  char client_id[kMaxClientIdLength];  // Null-terminated
  uint8_t checksum;                    // XOR of all bytes
};

/**
 * @brief Runtime configuration with IP addresses.
 */
struct RuntimeConfig {
  uint8_t device_ip[4];  // Static IP for this device
  uint8_t subnet[4];     // Subnet mask
  uint8_t gateway[4];    // Gateway IP
  uint8_t broker_ip[4];  // MQTT broker IP
  uint16_t broker_port;
  char username[kMaxUsernameLength];
  char password[kMaxPasswordLength];
  char client_id[kMaxClientIdLength];
  bool is_valid;
};

/**
 * @brief Configuration Manager for EEPROM-based persistent storage.
 *
 * Handles loading, saving, and validating MQTT broker configuration.
 * Automatically detects whether broker is an IP address or hostname.
 */
class ConfigManager {
 public:
  ConfigManager();
  ~ConfigManager() = default;

  /**
   * @brief Load configuration from EEPROM.
   * @return true if valid configuration was loaded, false if EEPROM empty/corrupt.
   */
  bool load();

  /**
   * @brief Save current configuration to EEPROM.
   * @return true if save was successful.
   */
  bool save();

  /**
   * @brief Reset configuration to defaults.
   */
  void reset_to_defaults();

  /**
   * @brief Check if configuration is valid.
   * @return true if configuration is valid and complete.
   */
  bool is_valid() const;

  /**
   * @brief Check if EEPROM contains valid configuration.
   * @return true if EEPROM has valid magic bytes and checksum.
   */
  bool has_stored_config() const;

  // Setters for IP addresses
  void set_device_ip(const uint8_t* ip);
  void set_subnet(const uint8_t* subnet);
  void set_gateway(const uint8_t* gateway);
  void set_broker_ip(const uint8_t* ip);
  void set_port(uint16_t port);
  void set_username(const char* username);
  void set_password(const char* password);
  void set_client_id(const char* client_id);

  // Getters
  const uint8_t* get_device_ip() const;
  const uint8_t* get_subnet() const;
  const uint8_t* get_gateway() const;
  const uint8_t* get_broker_ip() const;
  uint16_t get_port() const;
  const char* get_username() const;
  const char* get_password() const;
  const char* get_client_id() const;

  /**
   * @brief Get the runtime configuration.
   * @return Reference to runtime config structure.
   */
  const RuntimeConfig& get_runtime_config() const;

 private:
  RuntimeConfig config_;

  /**
   * @brief Calculate checksum for stored config.
   * @param config Stored config structure.
   * @return XOR checksum of all bytes except checksum field.
   */
  uint8_t calculate_checksum(const StoredConfig& config) const;

  /**
   * @brief Validate stored config magic bytes and checksum.
   * @param config Stored config to validate.
   * @return true if valid.
   */
  bool validate_config(const StoredConfig& config) const;

  /**
   * @brief Safe string copy with null termination.
   * @param dest Destination buffer.
   * @param src Source string.
   * @param max_len Maximum length including null terminator.
   */
  void safe_strcpy(char* dest, const char* src, uint8_t max_len);
};

}  // namespace Config

#endif  // PUBLIC_CONFIG_CONFIGMANAGER_H_
