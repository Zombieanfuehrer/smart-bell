#ifndef PUBLIC_CONFIG_LIGHTWEIGHTCONFIG_H_
#define PUBLIC_CONFIG_LIGHTWEIGHTCONFIG_H_

#include <stdint.h>

// Forward declarations
namespace serial {
class Interface;
#ifdef __AVR__
class UART;
#endif
}  // namespace serial

namespace Config {

// Minimal configuration structure (~230 bytes)
struct SmartBellConfig {
  // MQTT settings
  uint8_t broker_ip[4];
  uint16_t broker_port;
  char client_id[16];

  // MQTT Topics (Added for Bell Inputs and Chime control)
  char input1_topic[25];     // 24 chars + '\0'
  char input2_topic[25];     // 24 chars + '\0'
  char gong_base_topic[25];  // 24 chars + '\0'

  // Network settings (static only for simplicity)
  uint8_t device_ip[4];
  uint8_t subnet[4];
  uint8_t gateway[4];
  uint8_t mac[6];

  // Validation
  uint32_t magic;  // 0xBEEFCAFE if valid
};

class LightweightConfig {
 public:
  // Constructor takes generic serial interface
  explicit LightweightConfig(serial::Interface& uart);

  // Command processing
  bool process_command(const char* command);

  // Config access/storage
  void load();
  void save();
  void reset_to_defaults();

  const SmartBellConfig& config() const { return config_; }
  SmartBellConfig& mutable_config() { return config_; }

  // Returns true (and clears flag) if save() was called since last check.
  // Used by main loop to trigger MQTT connect after 'save' command.
  bool consume_save_flag() {
    if (save_flag_) {
      save_flag_ = false;
      return true;
    }
    return false;
  }

 private:
  SmartBellConfig config_;
  serial::Interface* uart_;
  bool save_flag_ = false;

  // Helper to parse "192.168.1.100" → uint8_t[4]
  bool parse_ip(const char* str, uint8_t* ip);

  // Helper to stream IP bytes to UART without stack buffers (saves flash space)
  void print_ip(const uint8_t* ip);

  // Helper to send short message
  void msg_ptr(const char* progmem_text);
};

}  // namespace Config

#endif  // PUBLIC_CONFIG_LIGHTWEIGHTCONFIG_H_
