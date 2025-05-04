#ifndef PUBLIC_ETERNET_W5500_W5500CLASS_H_
#define PUBLIC_ETHERNET_W5500_W5500CLASS_H_

#include "Serial/SPI.h"
#include "Ethernet/wizchip_conf.h"


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

class W5500Class
{
 public:
  W5500Class(serial::SPI *const SPI);
  ~W5500Class();

  void init();
  void soft_reset();
  void hard_reset(void (*callback)());

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
  static wiz_NetInfo netInfo_;
  static bool initialized_;

  static void cb_chip_select(void (*callback)());
  static void cb_chip_deselect(void (*callback)());
  static uint8_t cb_spi_read();
  static void cb_spi_write(uint8_t data);
  static void cb_spi_read_burst(uint8_t* pBuf, uint16_t len);
  static void cb_spi_write_burst(uint8_t* pBuf, uint16_t len);

  static W5500Class* instance_;
  static void (*cb_chip_select_)();
  static void (*cb_chip_deselect_)();
};



#endif // PUBLIC_ETHERNET_W5500_W5500CLASS_H_


