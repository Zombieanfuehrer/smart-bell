#ifndef PUBLIC_APP_SMARTBELLAPP_H_
#define PUBLIC_APP_SMARTBELLAPP_H_

#include <stdint.h>
#include "App/GongController.h"
#include "Config/ConfigManager.h"
#include "Config/UARTCommandParser.h"
#include "Ethernet/W5500/W5500Interface.h"
#include "Network/MQTTClient.h"
#include "Serial/Interface.h"
#include "SetupEXT_IN_Interrupt.h"

// Define to disable verbose logging (saves ~2-3KB flash)
#ifndef SMARTBELL_VERBOSE_LOG
#define SMARTBELL_VERBOSE_LOG 0
#endif

namespace App {

/**
 * @brief Application state machine states.
 */
enum class AppState : uint8_t {
  kInit = 0,        ///< Initial state
  kConfigCheck,     ///< Check EEPROM for valid configuration
  kConfigMode,      ///< UART configuration menu (if EEPROM empty)
  kNetworkInit,     ///< Initialize W5500 with static IP
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
 * @brief Button identifiers for the two doorbell buttons.
 */
enum class ButtonId : uint8_t {
  kFrontdoor = 0,  ///< INT0 / PD2
  kOffice = 1      ///< INT1 / PD3
};

/**
 * @brief MQTT topics used by SmartBell.
 */
struct SmartBellTopics {
  // Button event topics (publish)
  static constexpr const char* kFrontdoorActive = "smartbell/bellbutton/frontdoor/active";
  static constexpr const char* kFrontdoorInactive = "smartbell/bellbutton/frontdoor/inactive";
  static constexpr const char* kOfficeActive = "smartbell/bellbutton/office/active";
  static constexpr const char* kOfficeInactive = "smartbell/bellbutton/office/inactive";

  // Gong status topics (subscribe)
  static constexpr const char* kGongUpperfloorStatus = "smartbell/gong/upperfloor/status";
  static constexpr const char* kGongGroundfloorStatus = "smartbell/gong/groundfloor/status";

  // Test gong topics (subscribe)
  static constexpr const char* kTestGongUpperfloor = "smartbell/gong/upperfloor/testgong";
  static constexpr const char* kTestGongGroundfloor = "smartbell/gong/groundfloor/testgong";
  static constexpr const char* kTestGongBoth = "smartbell/gong/testgong";

  // Configuration topics (subscribe)
  static constexpr const char* kGongDuration = "smartbell/gong/duration";

  // Legacy topics
  static constexpr const char* kStatus = "smartbell/status";
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
   * @brief Handle doorbell button interrupt (call from INT0/INT1 ISR).
   * @param button Which button triggered the interrupt.
   */
  void on_button_interrupt(ButtonId button);

  /**
   * @brief Get current application state.
   * @return Current AppState.
   */
  AppState get_state() const;

  /**
   * @brief Get gong controller for external access.
   * @return Pointer to GongController.
   */
  GongController* get_gong_controller();

  /**
   * @brief Get configuration manager for external access.
   * @return Pointer to ConfigManager.
   */
  Config::ConfigManager* get_config_manager();

 private:
  // Core components
  Ethernet::W5500Interface* w5500_;
  serial::Interface* uart_;

  // Configuration - embedded to avoid dynamic allocation
  Config::ConfigManager config_manager_;
  Config::UARTCommandParser command_parser_;

  // Network client - embedded to avoid dynamic allocation
  SmartBell::MQTTClient mqtt_client_;

  // Gong controller - embedded
  GongController gong_controller_;

  // Button state structure
  struct ButtonState {
    volatile bool interrupt_pending;  ///< Interrupt occurred, needs processing
    bool last_state;                  ///< Last known pin state (true=HIGH/released)
    bool current_state;               ///< Current pin state after interrupt
  };

  ButtonState frontdoor_button_;
  ButtonState office_button_;

  // State
  AppState state_;
  AppState previous_state_;

  // Timing
  uint32_t state_enter_time_;
  uint32_t reconnect_start_time_;

  /**
   * @brief Transition to new state.
   * @param new_state Target state.
   */
  void transition_to(AppState new_state);

#if SMARTBELL_VERBOSE_LOG
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
#endif

  // State handlers
  void handle_init();
  void handle_config_check();
  void handle_config_mode();
  void handle_network_init();
  void handle_mqtt_connecting();
  void handle_running();
  void handle_reconnect_wait();
  void handle_error();

  /**
   * @brief Process button state changes and publish MQTT events.
   */
  void process_button_events();

  /**
   * @brief Publish button event to MQTT.
   * @param button Which button.
   * @param active true for active (pressed), false for inactive (released).
   */
  void publish_button_event(ButtonId button, bool active);

  /**
   * @brief Subscribe to all required MQTT topics.
   */
  void subscribe_to_topics();

  /**
   * @brief Handle received MQTT message.
   * @param message Received message data.
   */
  static void on_mqtt_message(const SmartBell::MQTTMessageData* message);

  /**
   * @brief Process received MQTT message.
   * @param topic Topic string.
   * @param payload Message payload.
   * @param length Payload length.
   */
  void process_mqtt_message(const char* topic, const uint8_t* payload, uint16_t length);

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
