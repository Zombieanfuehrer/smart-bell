#include "App/SmartBellApp.h"
#include <string.h>
#include "SetupWDT.h"
#include "System/TimerService.h"

namespace App {

// Static instance for callbacks
SmartBellApp* SmartBellApp::instance_ = nullptr;

SmartBellApp::SmartBellApp(Ethernet::W5500Interface* w5500, serial::Interface* uart)
    : w5500_(w5500),
      uart_(uart),
      command_parser_(nullptr),
      dhcp_client_(nullptr),
      dns_client_(nullptr),
      mqtt_client_(nullptr),
      state_(AppState::kInit),
      previous_state_(AppState::kInit),
      bell_state_(BellState::kEnabled),
      bell_triggered_(false),
      state_enter_time_(0),
      reconnect_start_time_(0),
      broker_ip_resolved_(false),
      ring_counter_(0) {
  memset(broker_ip_, 0, 4);
  instance_ = this;
}

SmartBellApp::~SmartBellApp() {
  delete command_parser_;
  delete dhcp_client_;
  delete dns_client_;
  delete mqtt_client_;

  if (instance_ == this) {
    instance_ = nullptr;
  }
}

void SmartBellApp::init() {
  log("[APP] SmartBell initializing...\r\n");

  // Create command parser
  command_parser_ = new Config::UARTCommandParser(uart_, &config_manager_);

  // Create network clients
  dhcp_client_ = new Network::DHCPClient(w5500_, uart_);
  dns_client_ = new Network::DNSClient(uart_);
  mqtt_client_ = new Network::MQTTClient(uart_);

  // Initialize MQTT client
  mqtt_client_->init();
  mqtt_client_->set_reconnect_interval(kReconnectIntervalSec);
  mqtt_client_->set_default_message_handler(on_mqtt_message);

  // Start state machine
  transition_to(AppState::kConfigCheck);

  log("[APP] Initialization complete\r\n");
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
    case AppState::kDHCPWait:
      handle_dhcp_wait();
      break;
    case AppState::kDNSResolve:
      handle_dns_resolve();
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

void SmartBellApp::on_bell_interrupt() { bell_triggered_ = true; }

AppState SmartBellApp::get_state() const { return state_; }

BellState SmartBellApp::get_bell_state() const { return bell_state_; }

bool SmartBellApp::is_bell_triggered() const { return bell_triggered_; }

void SmartBellApp::set_bell_state(BellState state) {
  bell_state_ = state;
  if (state == BellState::kEnabled) {
    log("[APP] Bell ENABLED\r\n");
  } else {
    log("[APP] Bell DISABLED\r\n");
  }
}

Config::ConfigManager* SmartBellApp::get_config_manager() { return &config_manager_; }

void SmartBellApp::transition_to(AppState new_state) {
  if (state_ != new_state) {
    log_state_transition(state_, new_state);
    previous_state_ = state_;
    state_ = new_state;
    state_enter_time_ = System::TimerService::seconds();
  }
}

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
    case AppState::kDHCPWait:
      return "DHCP_WAIT";
    case AppState::kDNSResolve:
      return "DNS_RESOLVE";
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
      transition_to(AppState::kDHCPWait);
    } else {
      log("[APP] Configuration incomplete\r\n");
      command_parser_->init(true);
      transition_to(AppState::kConfigMode);
    }
  } else {
    log("[APP] No valid configuration in EEPROM\r\n");
    command_parser_->init(true);
    transition_to(AppState::kConfigMode);
  }
}

void SmartBellApp::handle_config_mode() {
  // Process UART commands
  command_parser_->process();

  // Check if configuration was saved
  if (command_parser_->config_saved()) {
    command_parser_->clear_flags();

    if (config_manager_.is_valid()) {
      log("[APP] Configuration complete, starting network...\r\n");
      transition_to(AppState::kDHCPWait);
    }
  }
}

void SmartBellApp::handle_dhcp_wait() {
  static bool dhcp_initialized = false;

  if (!dhcp_initialized) {
    dhcp_client_->init();
    dhcp_initialized = true;
  }

  Network::DHCPStatus status = dhcp_client_->run();

  switch (status) {
    case Network::DHCPStatus::kIPLeased:
    case Network::DHCPStatus::kIPAssign:
    case Network::DHCPStatus::kIPChanged:
      // Apply configuration to W5500
      dhcp_client_->apply_config();

      // Get DNS server for hostname resolution
      uint8_t dns_server[4];
      dhcp_client_->get_dns(dns_server);
      dns_client_->set_dns_server(dns_server);
      dns_client_->init();

      // Check if broker is hostname or IP
      if (config_manager_.broker_is_hostname()) {
        transition_to(AppState::kDNSResolve);
      } else {
        // Broker is IP address, use directly
        config_manager_.get_broker_ip(broker_ip_);
        broker_ip_resolved_ = true;
        transition_to(AppState::kMQTTConnecting);
      }
      dhcp_initialized = false;  // Reset for potential reconnect
      break;

    case Network::DHCPStatus::kFailed:
      log("[APP] DHCP failed, retrying...\r\n");
      dhcp_initialized = false;
      // Stay in DHCP_WAIT and retry
      break;

    case Network::DHCPStatus::kRunning:
      // Still waiting
      break;

    default:
      break;
  }
}

void SmartBellApp::handle_dns_resolve() {
  log("[APP] Resolving broker hostname...\r\n");

  Network::DNSResult result = dns_client_->resolve(config_manager_.get_broker(), broker_ip_);

  if (result == Network::DNSResult::kSuccess) {
    broker_ip_resolved_ = true;
    transition_to(AppState::kMQTTConnecting);
  } else {
    log("[APP] DNS resolution failed\r\n");
    reconnect_start_time_ = System::TimerService::seconds();
    transition_to(AppState::kReconnectWait);
  }
}

void SmartBellApp::handle_mqtt_connecting() {
  if (!broker_ip_resolved_) {
    log("[APP] Error: Broker IP not resolved\r\n");
    transition_to(AppState::kError);
    return;
  }

  // Build MQTT config
  Network::MQTTConfig mqtt_config;
  memcpy(mqtt_config.broker_ip, broker_ip_, 4);
  mqtt_config.broker_port = config_manager_.get_port();
  mqtt_config.client_id = config_manager_.get_client_id();
  mqtt_config.keepalive_sec = kMQTTKeepAlive;
  mqtt_config.clean_session = true;

  // Build credentials if available
  Network::MQTTCredentials credentials;
  Network::MQTTCredentials* creds_ptr = nullptr;

  if (config_manager_.get_username()[0] != '\0') {
    credentials.username = config_manager_.get_username();
    credentials.password = config_manager_.get_password();
    creds_ptr = &credentials;
  }

  // Connect
  if (mqtt_client_->connect(mqtt_config, creds_ptr)) {
    // Subscribe to control topic
    if (mqtt_client_->subscribe(SmartBellTopics::kControl, on_mqtt_message,
                                Network::MQTTQoS::kQoS1)) {
      log("[APP] Subscribed to control topic\r\n");
    }

    // Publish initial status
    publish_status();

    transition_to(AppState::kRunning);
  } else {
    log("[APP] MQTT connection failed\r\n");
    reconnect_start_time_ = System::TimerService::seconds();
    transition_to(AppState::kReconnectWait);
  }
}

void SmartBellApp::handle_running() {
  // Process MQTT messages
  mqtt_client_->yield(100);

  // Check if bell was triggered
  if (bell_triggered_) {
    bell_triggered_ = false;

    if (bell_state_ == BellState::kEnabled) {
      publish_ring_event();
    } else {
      log("[APP] Bell triggered but disabled\r\n");
    }
  }

  // Check if connection was lost
  if (!mqtt_client_->is_connected()) {
    log("[APP] MQTT connection lost\r\n");
    reconnect_start_time_ = System::TimerService::seconds();
    transition_to(AppState::kReconnectWait);
  }

  // Also process any UART commands for runtime configuration
  if (uart_->is_read_data_available()) {
    command_parser_->process();
  }
}

void SmartBellApp::handle_reconnect_wait() {
  uint32_t elapsed = System::TimerService::seconds() - reconnect_start_time_;

  if (elapsed >= kReconnectIntervalSec) {
    log("[APP] Reconnect interval elapsed, retrying...\r\n");

    // Determine where to reconnect from
    if (!broker_ip_resolved_ && config_manager_.broker_is_hostname()) {
      transition_to(AppState::kDNSResolve);
    } else {
      transition_to(AppState::kMQTTConnecting);
    }
  }

  // Still process bell triggers during reconnect wait
  if (bell_triggered_) {
    bell_triggered_ = false;
    log("[APP] Bell triggered during reconnect (not published)\r\n");
  }
}

void SmartBellApp::handle_error() {
  // Log error periodically
  static uint32_t last_error_log = 0;
  uint32_t now = System::TimerService::seconds();

  if (now - last_error_log >= 10) {
    log("[APP] In error state, attempting recovery...\r\n");
    last_error_log = now;

    // Try to recover by going back to DHCP
    transition_to(AppState::kDHCPWait);
  }
}

void SmartBellApp::publish_ring_event() {
  ring_counter_++;

  log("[APP] Bell pressed! Publishing ring event...\r\n");

  // Build JSON payload
  // {"event":"ring","count":123}
  char payload[64];
  char* p = payload;

  // Manual JSON construction (no sprintf on AVR)
  const char* prefix = "{\"event\":\"ring\",\"count\":";
  while (*prefix)
    *p++ = *prefix++;

  // Convert counter to string
  char num_buf[12];
  uint32_t n = ring_counter_;
  int i = 0;
  do {
    num_buf[i++] = '0' + (n % 10);
    n /= 10;
  } while (n > 0);

  // Reverse number
  while (i > 0) {
    *p++ = num_buf[--i];
  }

  *p++ = '}';
  *p = '\0';

  if (mqtt_client_->publish_string(SmartBellTopics::kRing, payload, Network::MQTTQoS::kQoS1)) {
    log("[APP] Ring event published\r\n");
  } else {
    log("[APP] Failed to publish ring event\r\n");
  }
}

void SmartBellApp::publish_status() {
  log("[APP] Publishing status...\r\n");

  // Build JSON payload
  // {"state":"enabled"} or {"state":"disabled"}
  const char* payload;
  if (bell_state_ == BellState::kEnabled) {
    payload = "{\"state\":\"enabled\"}";
  } else {
    payload = "{\"state\":\"disabled\"}";
  }

  mqtt_client_->publish_string(SmartBellTopics::kStatus, payload, Network::MQTTQoS::kQoS0);
}

void SmartBellApp::on_mqtt_message(const Network::MQTTMessageData* message) {
  if (instance_ == nullptr) {
    return;
  }

  instance_->log("[APP] MQTT message received\r\n");
  instance_->process_control_command(message->payload, message->payload_length);
}

void SmartBellApp::process_control_command(const uint8_t* payload, uint16_t length) {
  // Simple command parsing
  // "enable" - enable bell
  // "disable" - disable bell

  // Null-terminate for comparison (temporary buffer)
  char cmd[16];
  uint16_t copy_len = (length < 15) ? length : 15;
  memcpy(cmd, payload, copy_len);
  cmd[copy_len] = '\0';

  // Convert to lowercase for comparison
  for (uint16_t i = 0; i < copy_len; i++) {
    if (cmd[i] >= 'A' && cmd[i] <= 'Z') {
      cmd[i] = cmd[i] + ('a' - 'A');
    }
  }

  if (strncmp(cmd, "enable", 6) == 0) {
    set_bell_state(BellState::kEnabled);
    publish_status();
  } else if (strncmp(cmd, "disable", 7) == 0) {
    set_bell_state(BellState::kDisabled);
    publish_status();
  } else {
    log("[APP] Unknown control command: ");
    log(cmd);
    log("\r\n");
  }
}

void SmartBellApp::log(const char* message) {
  if (uart_ != nullptr) {
    uart_->send_string(message);
  }
}

}  // namespace App
