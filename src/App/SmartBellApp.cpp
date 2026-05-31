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
      command_parser_(uart, &config_manager_),
      mqtt_client_(uart),
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

  // Initialize MQTT client
  mqtt_client_.init();
  mqtt_client_.set_reconnect_interval(kReconnectIntervalSec);
  mqtt_client_.set_default_message_handler(on_mqtt_message);

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

  if (config_manager_.load()) {
    log("[APP] Configuration loaded from EEPROM\r\n");

    // Check if broker is set
    if (config_manager_.is_valid()) {
      log("[APP] Configuration valid, starting network...\r\n");
      transition_to(AppState::kNetworkInit);
    } else {
      log("[APP] Configuration incomplete\r\n");
      command_parser_.init(true);
      transition_to(AppState::kConfigMode);
    }
  } else {
    log("[APP] No valid configuration in EEPROM\r\n");
    command_parser_.init(true);
    transition_to(AppState::kConfigMode);
  }
}

void SmartBellApp::handle_config_mode() {
  // Process UART commands
  command_parser_.process();

  // Check if configuration was saved
  if (command_parser_.config_saved()) {
    command_parser_.clear_flags();

    if (config_manager_.is_valid()) {
      log("[APP] Configuration complete, starting network...\r\n");
      transition_to(AppState::kNetworkInit);
    }
  }
}

void SmartBellApp::handle_network_init() {
  log("[APP] Configuring static IP...\r\n");

  // Get static IP configuration from ConfigManager
  const Config::RuntimeConfig& cfg = config_manager_.get_runtime_config();

  // Configure W5500 with static IP using proper types
  Ethernet::IpAddress ip;
  Ethernet::SubnetMask subnet;
  Ethernet::GatewayAddress gateway;

  memcpy(ip.addr, cfg.device_ip, 4);
  memcpy(subnet.addr, cfg.subnet, 4);
  memcpy(gateway.addr, cfg.gateway, 4);

  w5500_->set_IP(&ip);
  w5500_->set_subnet(&subnet);
  w5500_->set_gateway(&gateway);

  log("[APP] Network configured, connecting to MQTT...\r\n");
  transition_to(AppState::kMQTTConnecting);
}

void SmartBellApp::handle_mqtt_connecting() {
  // Build MQTT config from static IP settings
  SmartBell::MQTTConfig mqtt_config;
  memcpy(mqtt_config.broker_ip, config_manager_.get_broker_ip(), 4);
  mqtt_config.broker_port = config_manager_.get_port();
  mqtt_config.client_id = config_manager_.get_client_id();
  mqtt_config.keepalive_sec = kMQTTKeepAlive;
  mqtt_config.clean_session = true;

  // Build credentials if available
  SmartBell::MQTTCredentials credentials;
  SmartBell::MQTTCredentials* creds_ptr = nullptr;

  if (config_manager_.get_username()[0] != '\0') {
    credentials.username = config_manager_.get_username();
    credentials.password = config_manager_.get_password();
    creds_ptr = &credentials;
  }

  // Connect
  if (mqtt_client_.connect(mqtt_config, creds_ptr)) {
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
  // Process MQTT messages
  mqtt_client_.yield(100);

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
  if (mqtt_client_.publish_string(topic, "", SmartBell::MQTTQoS::kQoS1)) {
    log("[APP] Button event published\r\n");
  } else {
    log("[APP] Failed to publish button event\r\n");
  }
}

void SmartBellApp::subscribe_to_topics() {
  log("[APP] Subscribing to topics...\r\n");

  // Gong status topics
  mqtt_client_.subscribe(SmartBellTopics::kGongUpperfloorStatus, on_mqtt_message,
                         SmartBell::MQTTQoS::kQoS1);
  mqtt_client_.subscribe(SmartBellTopics::kGongGroundfloorStatus, on_mqtt_message,
                         SmartBell::MQTTQoS::kQoS1);

  // Test gong topics
  mqtt_client_.subscribe(SmartBellTopics::kTestGongUpperfloor, on_mqtt_message,
                         SmartBell::MQTTQoS::kQoS1);
  mqtt_client_.subscribe(SmartBellTopics::kTestGongGroundfloor, on_mqtt_message,
                         SmartBell::MQTTQoS::kQoS1);
  mqtt_client_.subscribe(SmartBellTopics::kTestGongBoth, on_mqtt_message,
                         SmartBell::MQTTQoS::kQoS1);

  // Duration configuration topic
  mqtt_client_.subscribe(SmartBellTopics::kGongDuration, on_mqtt_message,
                         SmartBell::MQTTQoS::kQoS1);

  log("[APP] Subscribed to all topics\r\n");
}

void SmartBellApp::on_mqtt_message(const SmartBell::MQTTMessageData* message) {
  if (instance_ == nullptr) {
    return;
  }

  instance_->process_mqtt_message(message->topic, message->payload, message->payload_length);
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