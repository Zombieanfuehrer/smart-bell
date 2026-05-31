#ifndef PUBLIC_NETWORK_DHCPCLIENT_H_
#define PUBLIC_NETWORK_DHCPCLIENT_H_

#include <stdint.h>
#include "Ethernet/EmbeddedSocketInterface.h"
#include "Ethernet/W5500/W5500Interface.h"
#include "Serial/Interface.h"

namespace SmartBell {

/**
 * @brief DHCP client status codes.
 */
enum class DHCPStatus : uint8_t {
  kFailed = 0,     ///< DHCP process failed
  kRunning = 1,    ///< DHCP is in progress
  kIPAssign = 2,   ///< First IP assignment
  kIPChanged = 3,  ///< IP address changed
  kIPLeased = 4,   ///< IP successfully leased
  kStopped = 5     ///< DHCP stopped
};

/**
 * @brief Network configuration obtained from DHCP.
 */
struct NetworkConfig {
  uint8_t ip[4];
  uint8_t subnet[4];
  uint8_t gateway[4];
  uint8_t dns[4];
  uint32_t lease_time;
};

/**
 * @brief C++ wrapper for WIZnet ioLibrary DHCP client.
 *
 * Provides automatic IP configuration via DHCP protocol.
 * Uses UDP socket 0 for DHCP communication.
 */
class DHCPClient {
 public:
  /// Default socket number for DHCP (UDP socket)
  static constexpr uint8_t kDHCPSocket = 0;

  /// Buffer size for DHCP messages
  static constexpr uint16_t kDHCPBufferSize = 548;

  /**
   * @brief Construct DHCP client.
   * @param w5500 W5500 interface for applying network configuration.
   * @param uart Optional UART for debug logging (can be nullptr).
   */
  DHCPClient(Ethernet::W5500Interface* w5500, serial::Interface* uart = nullptr);
  ~DHCPClient();

  /**
   * @brief Initialize DHCP client.
   * Must be called before run().
   */
  void init();

  /**
   * @brief Run DHCP state machine.
   * Call this repeatedly in main loop until IP is leased.
   * @return Current DHCP status.
   */
  DHCPStatus run();

  /**
   * @brief Stop DHCP client.
   */
  void stop();

  /**
   * @brief Check if IP has been successfully obtained.
   * @return true if DHCP lease is active.
   */
  bool has_ip() const;

  /**
   * @brief Get obtained network configuration.
   * @return Network configuration structure.
   */
  const NetworkConfig& get_config() const;

  /**
   * @brief Get assigned IP address.
   * @param ip Output buffer for 4-byte IP.
   */
  void get_ip(uint8_t* ip) const;

  /**
   * @brief Get assigned subnet mask.
   * @param subnet Output buffer for 4-byte subnet.
   */
  void get_subnet(uint8_t* subnet) const;

  /**
   * @brief Get assigned gateway.
   * @param gateway Output buffer for 4-byte gateway.
   */
  void get_gateway(uint8_t* gateway) const;

  /**
   * @brief Get assigned DNS server.
   * @param dns Output buffer for 4-byte DNS.
   */
  void get_dns(uint8_t* dns) const;

  /**
   * @brief Apply obtained configuration to W5500.
   */
  void apply_config();

 private:
  Ethernet::W5500Interface* w5500_;
  serial::Interface* uart_;

  uint8_t dhcp_buffer_[kDHCPBufferSize];
  NetworkConfig config_;
  bool initialized_;
  bool has_ip_;
  DHCPStatus last_status_;

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

  /**
   * @brief Update config from DHCP server response.
   */
  void update_config_from_dhcp();

  // Static callbacks for ioLibrary
  static DHCPClient* instance_;
  static void on_ip_assign();
  static void on_ip_update();
  static void on_ip_conflict();
};

}  // namespace SmartBell

#endif  // PUBLIC_NETWORK_DHCPCLIENT_H_
