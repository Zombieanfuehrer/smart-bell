#include "App/SmartBellApp.h"
#include <string.h>
#include "SetupWDT.h"
#include "System/TimerService.h"

#ifdef __AVR__
#include <avr/pgmspace.h>
#endif

namespace App {

// Static instance for callbacks
SmartBellApp* SmartBellApp::instance_ = nullptr;

SmartBellApp::SmartBellApp(Ethernet::W5500Interface* w5500, serial::Interface* uart)
    : w5500_(w5500),
      uart_(uart),
      config_manager_(static_cast<serial::UART*>(uart)),
      command_parser_(static_cast<serial::UART*>(uart), &config_manager_),
      mqtt_client_(static_cast<serial::UART*>(uart)),
      gong_controller_(),
      frontdoor_button_{false, true, true},  // interrupt_pending=false, last=HIGH, current=HIGH
      office_button_{false, true, true},
      state_(AppState::kInit),
      previous_state_(AppState::kInit),
      state_enter_time_(0),
      reconnect_start_time_(0) {
  instance_ = this;
}

SmartBellApp::~SmartBellApp() {
  if (instance_ == this) {
    instance_ = nullptr;
  }
}

void SmartBellApp::init() {
  log("[APP] Init\r\n");

  // MinimalMQTT initialization happens via constructor - no init() needed
  // No reconnect_interval setting - managed internally by state machine

  // Start state machine
  transition_to(AppState::kConfigCheck);
}

void SmartBellApp::loop() {
  // Reset watchdog
  configure_wdt::reset();

  // Run state handler
  switch (state_) {
    case AppState::kInit:
      handle_init();
      break;
    case AppState::kConfigCheck:
      handle_config_check();
      break;
    case AppState::kConfigMode:
      handle_config_mode();
      break;
    case AppState::kNetworkInit:
      handle_network_init();
      break;
    case AppState::kMQTTConnecting:
      handle_mqtt_connecting();
      break;
    case AppState::kRunning:
      handle_running();
      break;
    case AppState::kReconnectWait:
      handle_reconnect_wait();
      break;
    case AppState::kError:
      handle_error();
      break;
  }
}

void SmartBellApp::on_button_interrupt(ButtonId button) {
  // Called from ISR - just set flag, actual processing happens in loop
  if (button == ButtonId::kFrontdoor) {
    frontdoor_button_.interrupt_pending = true;
  } else {
    office_button_.interrupt_pending = true;
  }
}

AppState SmartBellApp::get_state() const { return state_; }

GongController* SmartBellApp::get_gong_controller() { return &gong_controller_; }

Config::ConfigManager* SmartBellApp::get_config_manager() { return &config_manager_; }

void SmartBellApp::transition_to(AppState new_state) {
  if (state_ != new_state) {
#if SMARTBELL_VERBOSE_LOG
    log_state_transition(state_, new_state);
#endif
    previous_state_ = state_;
    state_ = new_state;
    state_enter_time_ = System::TimerService::seconds();
  }
}

#if SMARTBELL_VERBOSE_LOG
void SmartBellApp::log_state_transition(AppState from, AppState to) {
  log("[APP] State: ");
  log(state_name(from));
  log(" -> ");
  log(state_name(to));
  log("\r\n");
}

const char* SmartBellApp::state_name(AppState state) const {
  switch (state) {
    case AppState::kInit:
      return "INIT";
    case AppState::kConfigCheck:
      return "CONFIG_CHECK";
    case AppState::kConfigMode:
      return "CONFIG_MODE";
    case AppState::kNetworkInit:
      return "NETWORK_INIT";
    case AppState::kMQTTConnecting:
      return "MQTT_CONNECTING";
    case AppState::kRunning:
      return "RUNNING";
    case AppState::kReconnectWait:
      return "RECONNECT_WAIT";
    case AppState::kError:
      return "ERROR";
    default:
      return "UNKNOWN";
  }
}
#endif

// State handlers

void SmartBellApp::handle_init() {
  // Should not reach here, init() transitions to CONFIG_CHECK
  transition_to(AppState::kConfigCheck);
}

void SmartBellApp::handle_config_check() {
  log("[APP] Checking EEPROM configuration...\r\n");

  // Try to load configuration from EEPROM
  if (config_manager_.load_from_eeprom()) {
    // Config loaded successfully - validate broker IP is not 0.0.0.0
    const Config::SmartBellConfig& cfg = config_manager_.get_config();
    if (cfg.mqtt_broker_ip[0] != 0 || cfg.mqtt_broker_ip[1] != 0 || cfg.mqtt_broker_ip[2] != 0 ||
        cfg.mqtt_broker_ip[3] != 0) {
      log("[APP] Valid config loaded\r\n");
      transition_to(AppState::kNetworkInit);
    } else {
      log("[APP] Invalid broker IP, entering config mode\r\n");
      config_manager_.load_defaults();
      transition_to(AppState::kConfigMode);
    }
  } else {
    // EEPROM empty or corrupted - enter configuration mode
    log("[APP] No valid config, entering config mode\r\n");
    config_manager_.load_defaults();
    transition_to(AppState::kConfigMode);
  }
}

void SmartBellApp::handle_config_mode() {
  // Process UART commands
  command_parser_.process();

  // Check if configuration is now complete (broker IP set)
  const Config::SmartBellConfig& cfg = config_manager_.get_config();
  if (cfg.mqtt_broker_ip[0] != 0 || cfg.mqtt_broker_ip[1] != 0 || cfg.mqtt_broker_ip[2] != 0 ||
      cfg.mqtt_broker_ip[3] != 0) {
    log("[APP] Configuration complete\r\n");
    config_manager_.save_to_eeprom();
    transition_to(AppState::kNetworkInit);
  }
}

void SmartBellApp::handle_network_init() {
  log("[APP] Configuring static IP...\r\n");

  // Get configuration
  const Config::SmartBellConfig& cfg = config_manager_.get_config();

  // Configure W5500 with static IP using proper types
  Ethernet::IpAddress ip;
  Ethernet::SubnetMask subnet;
  Ethernet::GatewayAddress gateway;

  memcpy(ip.addr, cfg.static_ip, 4);
  memcpy(subnet.addr, cfg.subnet_mask, 4);
  memcpy(gateway.addr, cfg.gateway, 4);

  w5500_->set_IP(&ip);
  w5500_->set_subnet(&subnet);
  w5500_->set_gateway(&gateway);

  log("[APP] Network configured, connecting to MQTT...\r\n");
  transition_to(AppState::kMQTTConnecting);
}

void SmartBellApp::handle_mqtt_connecting() {
  log("[APP] MQTT Connecting...\r\n");

  // Build MQTT::Config from ConfigManager
  const Config::SmartBellConfig& cfg = config_manager_.get_config();
  MQTT::Config mqtt_cfg;
  memcpy(mqtt_cfg.broker_ip, cfg.mqtt_broker_ip, 4);
  mqtt_cfg.broker_port = cfg.mqtt_port;
  strncpy(mqtt_cfg.client_id, cfg.mqtt_client_id, sizeof(mqtt_cfg.client_id) - 1);
  mqtt_cfg.client_id[sizeof(mqtt_cfg.client_id) - 1] = '\0';
  mqtt_cfg.keepalive = cfg.mqtt_keepalive;

  // Set credentials if configured
  if (cfg.mqtt_username[0] != '\0') {
    mqtt_cfg.use_auth = true;
    strncpy(mqtt_cfg.username, cfg.mqtt_username, sizeof(mqtt_cfg.username) - 1);
    mqtt_cfg.username[sizeof(mqtt_cfg.username) - 1] = '\0';
    strncpy(mqtt_cfg.password, cfg.mqtt_password, sizeof(mqtt_cfg.password) - 1);
    mqtt_cfg.password[sizeof(mqtt_cfg.password) - 1] = '\0';
  } else {
    mqtt_cfg.use_auth = false;
  }

  // Connect
  if (mqtt_client_.connect(mqtt_cfg)) {
    // Subscribe to all required topics
    subscribe_to_topics();

    transition_to(AppState::kRunning);
  } else {
    log("[APP] MQTT connection failed\r\n");
    reconnect_start_time_ = System::TimerService::seconds();
    transition_to(AppState::kReconnectWait);
  }
}

void SmartBellApp::handle_running() {
  // Process MQTT messages (keepalive, receive)
  mqtt_client_.loop();

  // Process button state changes and publish events
  process_button_events();

  // Update gong controller (handles auto-off timing)
  gong_controller_.update();

  // Check if connection was lost
  if (!mqtt_client_.is_connected()) {
    log("[APP] MQTT connection lost\r\n");
    reconnect_start_time_ = System::TimerService::seconds();
    transition_to(AppState::kReconnectWait);
  }

  // Also process any UART commands for runtime configuration
  if (uart_->is_read_data_available()) {
    command_parser_.process();
  }
}

void SmartBellApp::handle_reconnect_wait() {
  uint32_t elapsed = System::TimerService::seconds() - reconnect_start_time_;

  if (elapsed >= kReconnectIntervalSec) {
    log("[APP] Reconnect interval elapsed, retrying...\r\n");
    transition_to(AppState::kMQTTConnecting);
  }

  // Still process button events during reconnect (trigger gongs locally)
  process_button_events();
  gong_controller_.update();
}

void SmartBellApp::handle_error() {
  // Log error periodically
  static uint32_t last_error_log = 0;
  uint32_t now = System::TimerService::seconds();

  if (now - last_error_log >= 10) {
    log("[APP] In error state, attempting recovery...\r\n");
    last_error_log = now;

    // Try to recover by re-initializing network
    transition_to(AppState::kNetworkInit);
  }
}

void SmartBellApp::process_button_events() {
  // Process frontdoor button (INT0)
  if (frontdoor_button_.interrupt_pending) {
    frontdoor_button_.interrupt_pending = false;

    // Read current pin state (true = HIGH = released)
    bool current = external_pin_interrupt::read_INT0_state();
    frontdoor_button_.current_state = current;

    if (current != frontdoor_button_.last_state) {
      bool active = !current;  // LOW = pressed = active

      // Publish button event
      publish_button_event(ButtonId::kFrontdoor, active);

      // On button release (transition to inactive), trigger gongs
      if (!active) {
        gong_controller_.trigger(GongId::kBoth);
        log("[APP] Frontdoor released, triggering gongs\r\n");
      } else {
        log("[APP] Frontdoor pressed\r\n");
      }

      frontdoor_button_.last_state = current;
    }
  }

  // Process office button (INT1)
  if (office_button_.interrupt_pending) {
    office_button_.interrupt_pending = false;

    bool current = external_pin_interrupt::read_INT1_state();
    office_button_.current_state = current;

    if (current != office_button_.last_state) {
      bool active = !current;

      publish_button_event(ButtonId::kOffice, active);

      if (!active) {
        gong_controller_.trigger(GongId::kBoth);
        log("[APP] Office released, triggering gongs\r\n");
      } else {
        log("[APP] Office pressed\r\n");
      }

      office_button_.last_state = current;
    }
  }
}

void SmartBellApp::publish_button_event(ButtonId button, bool active) {
  const char* topic;

  if (button == ButtonId::kFrontdoor) {
    topic = active ? SmartBellTopics::kFrontdoorActive : SmartBellTopics::kFrontdoorInactive;
  } else {
    topic = active ? SmartBellTopics::kOfficeActive : SmartBellTopics::kOfficeInactive;
  }

  // Publish with empty payload (topic itself indicates the event)
  const uint8_t empty_payload = 0;
  if (mqtt_client_.publish(topic, &empty_payload, 0)) {
    log("[APP] Button event published\r\n");
  } else {
    log("[APP] Failed to publish button event\r\n");
  }
}

void SmartBellApp::subscribe_to_topics() {
  log("[APP] Subscribing to topics...\r\n");

  // MinimalMQTT supports max 2 subscriptions with single callback
  // For multiple topics, use wildcard or handle in callback
  // Subscribe to test gong topics (most important for user interaction)
  mqtt_client_.subscribe(SmartBellTopics::kTestGongUpperfloor, on_mqtt_message);
  mqtt_client_.subscribe(SmartBellTopics::kTestGongBoth, on_mqtt_message);

  // Note: Limited to 2 subscriptions - gong status, groundfloor, duration topics not subscribed
  // Consider MQTT topic wildcards (e.g., "smartbell/gong/#") if broker supports

  log("[APP] Subscribed to critical topics\r\n");
}

void SmartBellApp::on_mqtt_message(const char* topic, const uint8_t* payload,
                                   uint16_t payload_len) {
  if (instance_ == nullptr) {
    return;
  }

  instance_->process_mqtt_message(topic, payload, payload_len);
}

void SmartBellApp::process_mqtt_message(const char* topic, const uint8_t* payload,
                                        uint16_t length) {
  // Convert payload to string (with null termination)
  char payload_str[16];
  uint16_t copy_len = (length < 15) ? length : 15;
  memcpy(payload_str, payload, copy_len);
  payload_str[copy_len] = '\0';

  // Convert to lowercase for comparison
  for (uint16_t i = 0; i < copy_len; i++) {
    if (payload_str[i] >= 'A' && payload_str[i] <= 'Z') {
      payload_str[i] = payload_str[i] + ('a' - 'A');
    }
  }

  log("[APP] MQTT: ");
  log(topic);
  log(" = ");
  log(payload_str);
  log("\r\n");

  // Handle gong status topics
  if (strcmp(topic, SmartBellTopics::kGongUpperfloorStatus) == 0) {
    bool enabled = (strncmp(payload_str, "active", 6) == 0);
    gong_controller_.set_enabled(GongId::kUpperfloor, enabled);
    log(enabled ? "[APP] Upperfloor gong enabled\r\n" : "[APP] Upperfloor gong disabled\r\n");
  } else if (strcmp(topic, SmartBellTopics::kGongGroundfloorStatus) == 0) {
    bool enabled = (strncmp(payload_str, "active", 6) == 0);
    gong_controller_.set_enabled(GongId::kGroundfloor, enabled);
    log(enabled ? "[APP] Groundfloor gong enabled\r\n" : "[APP] Groundfloor gong disabled\r\n");
  }
  // Handle test gong topics (trigger regardless of payload)
  else if (strcmp(topic, SmartBellTopics::kTestGongUpperfloor) == 0) {
    gong_controller_.trigger(GongId::kUpperfloor, true);  // force=true ignores enabled state
    log("[APP] Test gong upperfloor\r\n");
  } else if (strcmp(topic, SmartBellTopics::kTestGongGroundfloor) == 0) {
    gong_controller_.trigger(GongId::kGroundfloor, true);
    log("[APP] Test gong groundfloor\r\n");
  } else if (strcmp(topic, SmartBellTopics::kTestGongBoth) == 0) {
    gong_controller_.trigger(GongId::kBoth, true);
    log("[APP] Test gong both\r\n");
  }
  // Handle duration configuration
  else if (strcmp(topic, SmartBellTopics::kGongDuration) == 0) {
    // Parse number from payload
    uint16_t duration = 0;
    for (uint16_t i = 0; i < copy_len; i++) {
      if (payload_str[i] >= '0' && payload_str[i] <= '9') {
        duration = duration * 10 + (payload_str[i] - '0');
        if (duration > 10000)
          break;
      } else {
        break;
      }
    }
    if (duration >= 100 && duration <= 10000) {
      gong_controller_.set_duration(duration);
      log("[APP] Gong duration updated\r\n");
    }
  }
}

void SmartBellApp::log(const char* message) {
#if SMARTBELL_VERBOSE_LOG
  if (uart_ != nullptr) {
    uart_->send_string(message);
  }
#else
  (void)message;  // Suppress unused warning
#endif
}

}  // namespace App