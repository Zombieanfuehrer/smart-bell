#ifndef PUBLIC_NETWORK_DNSCLIENT_H_
#define PUBLIC_NETWORK_DNSCLIENT_H_

#include <stdint.h>
#include "Serial/Interface.h"

namespace SmartBell {

/**
 * @brief DNS resolution result.
 */
enum class DNSResult : int8_t {
  kDomainTooLong = -1,  ///< Domain name exceeds MAX_DOMAIN_NAME
  kFailed = 0,          ///< Resolution failed (timeout or parse error)
  kSuccess = 1          ///< Resolution successful
};

/**
 * @brief C++ wrapper for WIZnet ioLibrary DNS client.
 *
 * Provides hostname resolution to IP address.
 * Uses UDP socket 1 for DNS queries.
 * DNS is blocking and can take up to MAX_DNS_RETRY * DNS_WAIT_TIME seconds (default 6s).
 */
class DNSClient {
 public:
  /// Default socket number for DNS (UDP socket)
  static constexpr uint8_t kDNSSocket = 1;

  /// Buffer size for DNS messages
  static constexpr uint16_t kDNSBufferSize = 256;

  /**
   * @brief Construct DNS client.
   * @param uart Optional UART for debug logging (can be nullptr).
   */
  explicit DNSClient(serial::Interface* uart = nullptr);
  ~DNSClient() = default;

  /**
   * @brief Initialize DNS client.
   * Must be called before resolve().
   */
  void init();

  /**
   * @brief Set DNS server address.
   * @param dns_ip 4-byte DNS server IP address.
   */
  void set_dns_server(const uint8_t* dns_ip);

  /**
   * @brief Resolve hostname to IP address.
   * @param hostname Domain name to resolve (e.g., "mqtt.example.com").
   * @param ip_out Output buffer for 4-byte resolved IP.
   * @return DNSResult indicating success or failure reason.
   * @note This function blocks for up to 6 seconds on failure.
   */
  DNSResult resolve(const char* hostname, uint8_t* ip_out);

  /**
   * @brief Get last resolved IP address.
   * @param ip_out Output buffer for 4-byte IP.
   * @return true if last resolution was successful.
   */
  bool get_last_ip(uint8_t* ip_out) const;

  /**
   * @brief Check if DNS client is initialized.
   * @return true if init() was called.
   */
  bool is_initialized() const;

 private:
  serial::Interface* uart_;

  uint8_t dns_buffer_[kDNSBufferSize];
  uint8_t dns_server_[4];
  uint8_t last_ip_[4];
  bool initialized_;
  bool has_result_;

  /**
   * @brief Log message to UART if available.
   * @param message Message to log.
   */
  void log(const char* message);

  /**
   * @brief Log IP address to UART.
   * @param label Label for the IP.
   * @param ip IP address bytes.
   */
  void log_ip(const char* label, const uint8_t* ip);
};

}  // namespace SmartBell

#endif  // PUBLIC_NETWORK_DNSCLIENT_H_
