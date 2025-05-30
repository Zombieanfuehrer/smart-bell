#ifndef PUBLIC_ETHERNET_W5500_W5500INTERFACE_H_
#define PUBLIC_ETHERNET_W5500_W5500INTERFACE_H_

#include <Serial/SPI.h>
#include <Serial/UART.h>

#include "Ethernet/wizchip_conf.h"

namespace Ethernet {

struct MacAddress {
  uint8_t addr[6];
};
struct IpAddress {
  uint8_t addr[4];
};
struct SubnetMask {
  uint8_t addr[4];
};
struct GatewayAddress {
  uint8_t addr[4];
};

struct W5500Callbacks {
  void (*hard_reset)() = nullptr;
  void (*chip_select)() = nullptr;
  void (*chip_deselect)() = nullptr;
};

class W5500Interface
{
 public:
  W5500Interface(serial::SPI *const SPI, W5500Callbacks register_callbacks);
  W5500Interface(serial::SPI *const SPI, W5500Callbacks register_callbacks, serial::UART *const UART_LOG);
  W5500Interface(serial::SPI *const SPI, serial::UART *const UART_LOG);

  ~W5500Interface() = default;


  void init();
  void soft_reset();
  void hard_reset();

  void set_network_config(const MacAddress* const mac, const IpAddress* const ip, 
    const SubnetMask* const subnet, const GatewayAddress* const gateway);

  void set_MAC(const MacAddress* const mac);
  void set_IP(const IpAddress* const ip);
  void set_subnet(const SubnetMask* const subnet);
  void set_gateway(const GatewayAddress* const gateway);

  void get_MAC(MacAddress* mac) const;
  void get_IP(IpAddress* ip) const;
  void get_subnet(SubnetMask* subnet) const;
  void get_gateway(GatewayAddress* gateway) const;

 private:
  serial::SPI *const spi_ = nullptr;
  serial::UART *const uart_log_ = nullptr;
  static wiz_NetInfo netInfo_;
  bool initialized_;

  static uint8_t cb_spi_read();
  static void cb_spi_write(uint8_t data);
  static void cb_spi_read_burst(uint8_t* pBuf, uint16_t len);
  static void cb_spi_write_burst(uint8_t* pBuf, uint16_t len);

  static W5500Interface* instance_;
  void (*cb_hard_reset_)() = nullptr;
  void (*cb_chip_select_)() = nullptr;
  void (*cb_chip_deselect_)() = nullptr;
};

} // namespace Ethernet

#endif // PUBLIC_ETHERNET_W5500_W5500INTERFACE_H_


