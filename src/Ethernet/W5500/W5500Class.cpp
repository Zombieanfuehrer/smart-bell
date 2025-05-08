#include <stdint.h>
#include <util/delay.h>
#include <string.h>

#include "Ethernet/W5500/W5500Class.h"
#include "Ethernet/wizchip_conf.h"
#include "Serial/SPI.h"
#include "Ethernet/W5500/w5500.h"

namespace Ethernet {

W5500Interface* W5500Interface::instance_ = nullptr;


W5500Interface::W5500Interface(serial::SPI *SPI)
 : spi_(SPI) {
  instance_ = this;
}

W5500Interface::W5500Interface(serial::SPI *const SPI, serial::UART *const UART_LOG)
: spi_(SPI), 
uart_log_(UART_LOG) {
  instance_ = this;
}

void W5500Interface::init()
{
  if (initialized_)
      return;

  this->soft_reset();
  reg_wizchip_cs_cbfunc(cb_chip_select_, cb_chip_deselect_);
  reg_wizchip_spi_cbfunc(cb_spi_read, cb_spi_write);
  reg_wizchip_spiburst_cbfunc(cb_spi_read_burst, cb_spi_write_burst);

  // W5500 initialisieren mit Standard-Socket-Puffergrößen
  // Socket 0: 4KB, Rest 1KB für TX und RX
  uint8_t memsize[2][8] = {
      {4, 1, 1, 1, 1, 1, 1, 1}, // TX Buffer
      {4, 1, 1, 1, 1, 1, 1, 1}  // RX Buffer
  };

  if (ctlwizchip(CW_INIT_WIZCHIP, reinterpret_cast<void *>(memsize)) == -1) {
    if (uart_log_) {
        uart_log_->send_string("WIZCHIP INIT FAILED");
    }
  }
}

void W5500Interface::soft_reset() {
  ctlwizchip(CW_RESET_WIZCHIP, nullptr);
  _delay_ms(100);
}

inline void W5500Interface::cb_chip_select(void (*callback)()) {
  cb_chip_select_ = callback;
}

inline void W5500Interface::cb_chip_deselect(void (*callback)()) {
  cb_chip_deselect_ = callback;
}

inline uint8_t W5500Interface::cb_spi_read() {
  return instance_->spi_->read_byte();
}

inline void W5500Interface::cb_spi_write(uint8_t data) {
  instance_->spi_->send(data);	
}

inline void W5500Interface::cb_spi_read_burst(uint8_t *pBuf, uint16_t len) {
  for (size_t i = 0; i < len; i++) {
    *pBuf = instance_->spi_->read_byte();
    pBuf++;
  }
}

inline void W5500Interface::cb_spi_write_burst(uint8_t *pBuf, uint16_t len) {
  instance_->spi_->send_bytes(pBuf, len);
}

void W5500Interface::set_network_config(const MacAddress *const mac, const IpAddress *const ip, const SubnetMask *const subnet, const GatewayAddress *const gateway)
{
  set_MAC(mac);
  set_IP(ip);
  set_subnet(subnet);
  set_gateway(gateway);
}

void W5500Interface::set_MAC(const MacAddress* const mac) {
  wizchip_getnetinfo(&netInfo_);
  memcpy(netInfo_.mac, mac->addr, 6);
  wizchip_setnetinfo(&netInfo_);
}

void W5500Interface::set_IP(const IpAddress* const ip) {
  wizchip_getnetinfo(&netInfo_);
  memcpy(netInfo_.ip, ip->addr, 4);
  wizchip_setnetinfo(&netInfo_);
}

void W5500Interface::set_subnet(const SubnetMask* const subnet) {
  wizchip_getnetinfo(&netInfo_);
  memcpy(netInfo_.sn, subnet->addr, 4);
  wizchip_setnetinfo(&netInfo_);
}

void W5500Interface::set_gateway(const GatewayAddress* const gateway) {
  wizchip_getnetinfo(&netInfo_);
  memcpy(netInfo_.gw, gateway->addr, 4);
  wizchip_setnetinfo(&netInfo_);
}

void W5500Interface::get_MAC(MacAddress* mac) const {
  wizchip_getnetinfo(&netInfo_);
  memcpy(mac->addr, netInfo_.mac, 6);
}

void W5500Interface::get_IP(IpAddress* ip) const {
  wizchip_getnetinfo(&netInfo_);
  memcpy(ip->addr, netInfo_.ip, 4);
}

void W5500Interface::get_subnet(SubnetMask* subnet) const {
  wizchip_getnetinfo(&netInfo_);
  memcpy(subnet->addr, netInfo_.sn, 4);
}

void W5500Interface::get_gateway(GatewayAddress* gateway) const {
  wizchip_getnetinfo(&netInfo_);
  memcpy(gateway->addr, netInfo_.gw, 4);
}

} // namespace Ethernet
