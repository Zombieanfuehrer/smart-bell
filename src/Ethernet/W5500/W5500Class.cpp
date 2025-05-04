#include <stdint.h>
#include <util/delay.h>
#include <string.h>

#include "Ethernet/W5500/W5500Class.h"
#include "Ethernet/wizchip_conf.h"
#include "Serial/SPI.h"
#include "Ethernet/W5500/w5500.h"

W5500Class* W5500Class::instance_ = nullptr;


W5500Class::W5500Class(serial::SPI *SPI) : spi_(SPI) {
  instance_ = this;
}

void W5500Class::init() {
  if (initialized_) return;

  this->soft_reset();
  reg_wizchip_cs_cbfunc(cb_chip_select_, cb_chip_deselect_);
  reg_wizchip_spi_cbfunc(cb_spi_read, cb_spi_write);

  uint8_t tx_mem_conf[8] = {4, 1, 1, 1, 1, 1, 1, 1};  // Socket 0: 4KB, Rest 1KB
  uint8_t rx_mem_conf[8] = {4, 1, 1, 1, 1, 1, 1, 1};

  // W5500 initialisieren mit Standard-Socket-Puffergrößen
  // Socket 0: 4KB, Rest 1KB für TX und RX
  uint8_t memsize[2][8] = {
    {4, 1, 1, 1, 1, 1, 1, 1},  // TX Buffer
    {4, 1, 1, 1, 1, 1, 1, 1}   // RX Buffer
  };

  wizchip_init(memsize[0], memsize[1]);
}

void W5500Class::soft_reset() {
  WIZCHIP_WRITE(MR, MR_RST);
  _delay_ms(100);
}

inline void W5500Class::cb_chip_select(void (*callback)()) {
  cb_chip_select_ = callback;
}

inline void W5500Class::cb_chip_deselect(void (*callback)()) {
  cb_chip_deselect_ = callback;
}

inline uint8_t W5500Class::cb_spi_read() {
  return instance_->spi_->read_byte();
}

inline void W5500Class::cb_spi_write(uint8_t data) {
  instance_->spi_->send(data);	
}

inline void W5500Class::cb_spi_read_burst(uint8_t *pBuf, uint16_t len) {
  for (size_t i = 0; i < len; i++) {
    *pBuf = instance_->spi_->read_byte();
    pBuf++;
  }
}

inline void W5500Class::cb_spi_write_burst(uint8_t *pBuf, uint16_t len) {
  instance_->spi_->send_bytes(pBuf, len);
}

void W5500Class::set_network_config(const MacAddress *const mac, const IpAddress *const ip, const SubnetMask *const subnet, const GatewayAddress *const gateway)
{
  set_MAC(mac);
  set_IP(ip);
  set_subnet(subnet);
  set_gateway(gateway);
}

void W5500Class::set_MAC(const MacAddress* const mac) {
  wizchip_getnetinfo(&netInfo_);
  memcpy(netInfo_.mac, mac->addr, 6);
  wizchip_setnetinfo(&netInfo_);
}

void W5500Class::set_IP(const IpAddress* const ip) {
  wizchip_getnetinfo(&netInfo_);
  memcpy(netInfo_.ip, ip->addr, 4);
  wizchip_setnetinfo(&netInfo_);
}

void W5500Class::set_subnet(const SubnetMask* const subnet) {
  wizchip_getnetinfo(&netInfo_);
  memcpy(netInfo_.sn, subnet->addr, 4);
  wizchip_setnetinfo(&netInfo_);
}

void W5500Class::set_gateway(const GatewayAddress* const gateway) {
  wizchip_getnetinfo(&netInfo_);
  memcpy(netInfo_.gw, gateway->addr, 4);
  wizchip_setnetinfo(&netInfo_);
}

void W5500Class::get_MAC(MacAddress* mac) const {
  wizchip_getnetinfo(&netInfo_);
  memcpy(mac->addr, netInfo_.mac, 6);
}

void W5500Class::get_IP(IpAddress* ip) const {
  wizchip_getnetinfo(&netInfo_);
  memcpy(ip->addr, netInfo_.ip, 4);
}

void W5500Class::get_subnet(SubnetMask* subnet) const {
  wizchip_getnetinfo(&netInfo_);
  memcpy(subnet->addr, netInfo_.sn, 4);
}

void W5500Class::get_gateway(GatewayAddress* gateway) const {
  wizchip_getnetinfo(&netInfo_);
  memcpy(gateway->addr, netInfo_.gw, 4);
}