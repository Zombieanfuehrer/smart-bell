#include "Network/DNSClient.h"
#include <string.h>
#include "SetupWDT.h"

// Include ioLibrary DNS header
extern "C" {
#include "dns.h"
}

namespace Network {

DNSClient::DNSClient(serial::Interface* uart)
    : uart_(uart), initialized_(false), has_result_(false) {
  memset(dns_buffer_, 0, kDNSBufferSize);
  memset(dns_server_, 0, 4);
  memset(last_ip_, 0, 4);
}

void DNSClient::init() {
  log("[DNS] Initializing...\r\n");

  // Initialize DNS with socket and buffer
  DNS_init(kDNSSocket, dns_buffer_);

  initialized_ = true;
  has_result_ = false;

  log("[DNS] Ready\r\n");
}

void DNSClient::set_dns_server(const uint8_t* dns_ip) {
  memcpy(dns_server_, dns_ip, 4);
  log_ip("[DNS] Server set to: ", dns_server_);
}

DNSResult DNSClient::resolve(const char* hostname, uint8_t* ip_out) {
  if (!initialized_) {
    log("[DNS] Error: Not initialized\r\n");
    return DNSResult::kFailed;
  }

  if (dns_server_[0] == 0 && dns_server_[1] == 0 && dns_server_[2] == 0 && dns_server_[3] == 0) {
    log("[DNS] Error: No DNS server configured\r\n");
    return DNSResult::kFailed;
  }

  log("[DNS] Resolving: ");
  log(hostname);
  log("\r\n");

  // Pause WDT - DNS can block for up to 6 seconds
  configure_wdt::pause();

  // Perform DNS query
  // Cast away const for ioLibrary API (it doesn't modify the hostname)
  int8_t result =
      DNS_run(dns_server_, reinterpret_cast<uint8_t*>(const_cast<char*>(hostname)), ip_out);

  configure_wdt::resume();
  configure_wdt::reset();

  DNSResult status = static_cast<DNSResult>(result);

  switch (status) {
    case DNSResult::kSuccess:
      memcpy(last_ip_, ip_out, 4);
      has_result_ = true;
      log_ip("[DNS] Resolved to: ", ip_out);
      break;

    case DNSResult::kFailed:
      log("[DNS] Resolution failed (timeout or parse error)\r\n");
      has_result_ = false;
      break;

    case DNSResult::kDomainTooLong:
      log("[DNS] Error: Domain name too long\r\n");
      has_result_ = false;
      break;
  }

  return status;
}

bool DNSClient::get_last_ip(uint8_t* ip_out) const {
  if (!has_result_) {
    return false;
  }
  memcpy(ip_out, last_ip_, 4);
  return true;
}

bool DNSClient::is_initialized() const { return initialized_; }

void DNSClient::log(const char* message) {
  if (uart_ != nullptr) {
    uart_->send_string(message);
  }
}

void DNSClient::log_ip(const char* label, const uint8_t* ip) {
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
}

}  // namespace Network
