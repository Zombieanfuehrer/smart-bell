#ifndef PUBLIC_APP_SMARTBELLAPP_H_
#define PUBLIC_APP_SMARTBELLAPP_H_

#include <stdint.h>
#include "Config/ConfigManager.h"
#include "Config/UARTCommandParser.h"
#include "Ethernet/W5500/W5500Interface.h"
#include "Network/DHCPClient.h"
#include "Network/DNSClient.h"
#include "Network/MQTTClient.h"
#include "Serial/Interface.h"

namespace App {

/**
 * @brief Application state machine states.
 */
enum class AppState : uint8_t {
  kInit = 0,        ///< Initial state
  kConfigCheck,     ///< Check EEPROM for valid configuration
  kConfigMode,      ///< UART configuration menu (if EEPROM empty)
  kDHCPWait,        ///< Waiting for DHCP IP assignment
  kDNSResolve,      ///< Resolving broker hostname via DNS
  kMQTTConnecting,  ///< Connecting to MQTT broker
  kRunning,         ///< Normal operation - publish/subscribe active
  kReconnectWait,   ///< Waiting before reconnection attempt
  kError            ///< Error state
};

/**
 * @brief Bell state (enabled/disabled by MQTT control).
 */
enum class BellState : uint8_t { kEnabled = 0, kDisabled };

/**
 * @brief MQTT topics used by SmartBell.
 */
struct SmartBellTopics {
  static constexpr const char* kRing = "smartbell/ring";
  static constexpr const char* kStatus = "smartbell/status";
  static constexpr const char* kControl = "smartbell/control";
};

/**
 * @brief SmartBell main application class.
 *
 * Implements the complete state machine for:
 * - UART-based configuration on first boot
 * - DHCP network configuration
 * - DNS resolution for broker hostname
 * - MQTT connection with authentication
 * - Publishing doorbell events
 * - Subscribing to control commands
 * - Automatic reconnection
 */
class SmartBellApp {
 public:
  /// Reconnect interval in seconds
  static constexpr uint32_t kReconnectIntervalSec = 30;

  /// MQTT keep-alive interval in seconds
  static constexpr uint16_t kMQTTKeepAlive = 60;

  /**
   * @brief Construct SmartBell application.
   * @param w5500 W5500 Ethernet interface.
   * @param uart UART interface for logging and configuration.
   */
  SmartBellApp(Ethernet::W5500Interface* w5500, serial::Interface* uart);
  ~SmartBellApp();

  /**
   * @brief Initialize application.
   * Sets up all components and loads configuration.
   */
  void init();

  /**
   * @brief Main loop iteration.
   * Call this repeatedly in the main while(1) loop.
   */
  void loop();

  /**
   * @brief Handle doorbell button press (call from INT0 ISR).
   * Sets the bell_triggered_ flag for processing in loop().
   */
  void on_bell_interrupt();

  /**
   * @brief Get current application state.
   * @return Current AppState.
   */
  AppState get_state() const;

  /**
   * @brief Get current bell state.
   * @return Current BellState (enabled/disabled).
   */
  BellState get_bell_state() const;

  /**
   * @brief Check if bell is triggered and waiting to be published.
   * @return true if bell event pending.
   */
  bool is_bell_triggered() const;

  /**
   * @brief Manually set bell state.
   * @param state New bell state.
   */
  void set_bell_state(BellState state);

  /**
   * @brief Get configuration manager for external access.
   * @return Pointer to ConfigManager.
   */
  Config::ConfigManager* get_config_manager();

 private:
  // Core components
  Ethernet::W5500Interface* w5500_;
  serial::Interface* uart_;

  // Configuration
  Config::ConfigManager config_manager_;
  Config::UARTCommandParser* command_parser_;

  // Network clients
  Network::DHCPClient* dhcp_client_;
  Network::DNSClient* dns_client_;
  Network::MQTTClient* mqtt_client_;

  // State
  AppState state_;
  AppState previous_state_;
  BellState bell_state_;
  volatile bool bell_triggered_;

  // Timing
  uint32_t state_enter_time_;
  uint32_t reconnect_start_time_;

  // Resolved broker IP
  uint8_t broker_ip_[4];
  bool broker_ip_resolved_;

  // Ring event counter for timestamp
  uint32_t ring_counter_;

  /**
   * @brief Transition to new state.
   * @param new_state Target state.
   */
  void transition_to(AppState new_state);

  /**
   * @brief Log state transition.
   * @param from Previous state.
   * @param to New state.
   */
  void log_state_transition(AppState from, AppState to);

  /**
   * @brief Get state name for logging.
   * @param state State to convert.
   * @return State name string.
   */
  const char* state_name(AppState state) const;

  // State handlers
  void handle_init();
  void handle_config_check();
  void handle_config_mode();
  void handle_dhcp_wait();
  void handle_dns_resolve();
  void handle_mqtt_connecting();
  void handle_running();
  void handle_reconnect_wait();
  void handle_error();

  /**
   * @brief Publish doorbell ring event.
   */
  void publish_ring_event();

  /**
   * @brief Publish status message.
   */
  void publish_status();

  /**
   * @brief Handle received MQTT control message.
   * @param message Received message data.
   */
  static void on_mqtt_message(const Network::MQTTMessageData* message);

  /**
   * @brief Process control command from MQTT.
   * @param payload Command payload.
   * @param length Payload length.
   */
  void process_control_command(const uint8_t* payload, uint16_t length);

  /**
   * @brief Log message to UART.
   * @param message Message to log.
   */
  void log(const char* message);

  // Static instance for callbacks
  static SmartBellApp* instance_;
};

}  // namespace App

#endif  // PUBLIC_APP_SMARTBELLAPP_H_
