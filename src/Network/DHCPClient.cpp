#include "Network/DHCPClient.h"
#include <string.h>
#include "SetupWDT.h"

#ifndef SMARTBELL_VERBOSE_LOG
#define SMARTBELL_VERBOSE_LOG 0
#endif

// Include ioLibrary DHCP header
extern "C" {
#include "dhcp.h"
}

namespace SmartBell {

// Static instance pointer for callbacks
DHCPClient* DHCPClient::instance_ = nullptr;

DHCPClient::DHCPClient(Ethernet::W5500Interface* w5500, serial::Interface* uart)
    : w5500_(w5500),
      uart_(uart),
      initialized_(false),
      has_ip_(false),
      last_status_(DHCPStatus::kFailed) {
  memset(&config_, 0, sizeof(NetworkConfig));
  memset(dhcp_buffer_, 0, kDHCPBufferSize);
  instance_ = this;
}

DHCPClient::~DHCPClient() {
  if (initialized_) {
    stop();
  }
  if (instance_ == this) {
    instance_ = nullptr;
  }
}

void DHCPClient::init() {
  log("[DHCP] Initializing...\r\n");

  // Register callbacks
  reg_dhcp_cbfunc(on_ip_assign, on_ip_update, on_ip_conflict);

  // Initialize DHCP with socket and buffer
  DHCP_init(kDHCPSocket, dhcp_buffer_);

  initialized_ = true;
  has_ip_ = false;
  last_status_ = DHCPStatus::kRunning;

  log("[DHCP] Discovering...\r\n");
}

DHCPStatus DHCPClient::run() {
  if (!initialized_) {
    return DHCPStatus::kFailed;
  }

  // Reset watchdog before potentially blocking operation
  configure_wdt::reset();

  // Run DHCP state machine
  uint8_t result = DHCP_run();

  // Map ioLibrary result to our enum
  DHCPStatus status = static_cast<DHCPStatus>(result);

  // Log state changes
  if (status != last_status_) {
    switch (status) {
      case DHCPStatus::kRunning:
        // Already logged during init
        break;
      case DHCPStatus::kIPAssign:
        log("[DHCP] IP assigned\r\n");
        update_config_from_dhcp();
        log_ip("[DHCP] IP: ", config_.ip);
        log_ip("[DHCP] Gateway: ", config_.gateway);
        log_ip("[DHCP] DNS: ", config_.dns);
        has_ip_ = true;
        break;
      case DHCPStatus::kIPChanged:
        log("[DHCP] IP changed\r\n");
        update_config_from_dhcp();
        log_ip("[DHCP] New IP: ", config_.ip);
        has_ip_ = true;
        break;
      case DHCPStatus::kIPLeased:
        if (!has_ip_) {
          update_config_from_dhcp();
          log_ip("[DHCP] IP: ", config_.ip);
          has_ip_ = true;
        }
        break;
      case DHCPStatus::kFailed:
        log("[DHCP] Failed\r\n");
        has_ip_ = false;
        break;
      case DHCPStatus::kStopped:
        log("[DHCP] Stopped\r\n");
        break;
    }
    last_status_ = status;
  }

  configure_wdt::reset();
  return status;
}

void DHCPClient::stop() {
  if (initialized_) {
    DHCP_stop();
    initialized_ = false;
    log("[DHCP] Stopped\r\n");
  }
}

bool DHCPClient::has_ip() const { return has_ip_; }

const NetworkConfig& DHCPClient::get_config() const { return config_; }

void DHCPClient::get_ip(uint8_t* ip) const { memcpy(ip, config_.ip, 4); }

void DHCPClient::get_subnet(uint8_t* subnet) const { memcpy(subnet, config_.subnet, 4); }

void DHCPClient::get_gateway(uint8_t* gateway) const { memcpy(gateway, config_.gateway, 4); }

void DHCPClient::get_dns(uint8_t* dns) const { memcpy(dns, config_.dns, 4); }

void DHCPClient::apply_config() {
  if (!has_ip_ || w5500_ == nullptr) {
    return;
  }

  log("[DHCP] Applying network configuration...\r\n");

  Ethernet::IpAddress ip;
  Ethernet::SubnetMask subnet;
  Ethernet::GatewayAddress gateway;

  memcpy(ip.addr, config_.ip, 4);
  memcpy(subnet.addr, config_.subnet, 4);
  memcpy(gateway.addr, config_.gateway, 4);

  w5500_->set_IP(&ip);
  w5500_->set_subnet(&subnet);
  w5500_->set_gateway(&gateway);

  log("[DHCP] Configuration applied\r\n");
}

void DHCPClient::update_config_from_dhcp() {
  getIPfromDHCP(config_.ip);
  getSNfromDHCP(config_.subnet);
  getGWfromDHCP(config_.gateway);
  getDNSfromDHCP(config_.dns);
  config_.lease_time = getDHCPLeasetime();
}

void DHCPClient::log(const char* message) {
#if SMARTBELL_VERBOSE_LOG
  if (uart_ != nullptr) {
    uart_->send_string(message);
  }
#else
  (void)message;
#endif
}

void DHCPClient::log_ip(const char* label, const uint8_t* ip) {
#if SMARTBELL_VERBOSE_LOG
  if (uart_ == nullptr)
    return;

  uart_->send_string(label);

  char buffer[4];
  for (int i = 0; i < 4; i++) {
    // Convert byte to string
    uint8_t val = ip[i];
    int pos = 0;
    if (val >= 100) {
      buffer[pos++] = '0' + (val / 100);
      val %= 100;
      buffer[pos++] = '0' + (val / 10);
      val %= 10;
    } else if (val >= 10) {
      buffer[pos++] = '0' + (val / 10);
      val %= 10;
    }
    buffer[pos++] = '0' + val;
    buffer[pos] = '\0';

    uart_->send_string(buffer);
    if (i < 3) {
      uart_->send('.');
    }
  }
  uart_->send_string("\r\n");
#else
  (void)label;
  (void)ip;
#endif
}

// Static callbacks
void DHCPClient::on_ip_assign() {
  if (instance_ != nullptr) {
    instance_->log("[DHCP] Callback: IP assigned\r\n");
  }
  // Default behavior: set network info to W5500
  // This is done in run() after getting the values
}

void DHCPClient::on_ip_update() {
  if (instance_ != nullptr) {
    instance_->log("[DHCP] Callback: IP updated\r\n");
  }
}

void DHCPClient::on_ip_conflict() {
  if (instance_ != nullptr) {
    instance_->log("[DHCP] Callback: IP conflict detected!\r\n");
  }
}

}  // namespace SmartBell
