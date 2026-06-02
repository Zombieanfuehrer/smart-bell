#include <stdint.h>
#include <string.h>
#include <util/delay.h>

#ifdef __AVR__
#include <avr/io.h>
#endif

#include "Ethernet/W5500/w5500.h"
#include "Ethernet/wizchip_conf.h"
#include "Serial/SPI.h"
#include "W5500Interface.h"

namespace Ethernet {

W5500Interface *W5500Interface::instance_ = nullptr;

wiz_NetInfo W5500Interface::netInfo_;

W5500Interface::W5500Interface(serial::SPI *SPI, W5500Callbacks register_callbacks)
    : spi_(SPI), initialized_(false), uart_log_(nullptr) {
  instance_ = this;
  if (register_callbacks.hard_reset) {
    cb_hard_reset_ = register_callbacks.hard_reset;
  }
  if (register_callbacks.chip_select) {
    cb_chip_select_ = register_callbacks.chip_select;
  }
  if (register_callbacks.chip_deselect) {
    cb_chip_deselect_ = register_callbacks.chip_deselect;
  }
}

W5500Interface::W5500Interface(serial::SPI *const SPI, W5500Callbacks register_callbacks,
                               serial::UART *const UART_LOG)
    : spi_(SPI), initialized_(false), uart_log_(UART_LOG) {
  instance_ = this;
  if (register_callbacks.hard_reset) {
    cb_hard_reset_ = register_callbacks.hard_reset;
  }
  if (register_callbacks.chip_select) {
    cb_chip_select_ = register_callbacks.chip_select;
  }
  if (register_callbacks.chip_deselect) {
    cb_chip_deselect_ = register_callbacks.chip_deselect;
  }
}

void W5500Interface::init() {
  if (initialized_)
    return;

  // Register SPI/CS callbacks BEFORE any SPI communication
  reg_wizchip_cs_cbfunc(cb_chip_select_, cb_chip_deselect_);
  reg_wizchip_spi_cbfunc(cb_spi_read, cb_spi_write);
  reg_wizchip_spiburst_cbfunc(cb_spi_read_burst, cb_spi_write_burst);

  this->soft_reset();

  // Socket 0: 4KB TX/RX, remaining sockets: 1KB each
  uint8_t memsize[2][8] = {{4, 1, 1, 1, 1, 1, 1, 1}, {4, 1, 1, 1, 1, 1, 1, 1}};

  if (ctlwizchip(CW_INIT_WIZCHIP, reinterpret_cast<void *>(memsize)) == -1) {
    if (uart_log_) {
      uart_log_->send_string("WIZCHIP INIT FAILED");
    }
    return;
  }

  initialized_ = true;
}

void W5500Interface::soft_reset() {
  ctlwizchip(CW_RESET_WIZCHIP, nullptr);
  _delay_ms(100);
}

void W5500Interface::hard_reset() {
  if (cb_hard_reset_) {
    cb_hard_reset_();
  } else {
    if (uart_log_) {
      uart_log_->send_string("No hard reset callback defined");
    }
  }
  _delay_ms(100);
  initialized_ = false;
}

// Direct SPDR polling: SPIE is disabled by chip_select callback before any of
// these are called, so no ISR interference. Read SPDR after polling SPIF to
// clear the flag for the next transfer (ATmega328P datasheet, sec. 18.5).
inline uint8_t W5500Interface::cb_spi_read() {
  SPDR = 0xFF;  // dummy byte to generate clock pulses
  while (!(SPSR & (1 << SPIF)))
    ;
  return SPDR;
}

inline void W5500Interface::cb_spi_write(uint8_t data) {
  SPDR = data;
  while (!(SPSR & (1 << SPIF)))
    ;
  (void)SPDR;  // clear SPIF
}

inline void W5500Interface::cb_spi_read_burst(uint8_t *pBuf, uint16_t len) {
  for (uint16_t i = 0; i < len; i++) {
    SPDR = 0xFF;
    while (!(SPSR & (1 << SPIF)))
      ;
    pBuf[i] = SPDR;
  }
}

inline void W5500Interface::cb_spi_write_burst(uint8_t *pBuf, uint16_t len) {
  for (uint16_t i = 0; i < len; i++) {
    SPDR = pBuf[i];
    while (!(SPSR & (1 << SPIF)))
      ;
    (void)SPDR;  // clear SPIF
  }
}

void W5500Interface::set_network_config(const MacAddress *const mac, const IpAddress *const ip,
                                        const SubnetMask *const subnet,
                                        const GatewayAddress *const gateway) {
  set_MAC(mac);
  set_IP(ip);
  set_subnet(subnet);
  set_gateway(gateway);
}

void W5500Interface::set_MAC(const MacAddress *const mac) {
  wizchip_getnetinfo(&netInfo_);
  memcpy(netInfo_.mac, mac->addr, 6);
  wizchip_setnetinfo(&netInfo_);
}

void W5500Interface::set_IP(const IpAddress *const ip) {
  wizchip_getnetinfo(&netInfo_);
  memcpy(netInfo_.ip, ip->addr, 4);
  wizchip_setnetinfo(&netInfo_);
}

void W5500Interface::set_subnet(const SubnetMask *const subnet) {
  wizchip_getnetinfo(&netInfo_);
  memcpy(netInfo_.sn, subnet->addr, 4);
  wizchip_setnetinfo(&netInfo_);
}

void W5500Interface::set_gateway(const GatewayAddress *const gateway) {
  wizchip_getnetinfo(&netInfo_);
  memcpy(netInfo_.gw, gateway->addr, 4);
  wizchip_setnetinfo(&netInfo_);
}

void W5500Interface::get_MAC(MacAddress *mac) const {
  wizchip_getnetinfo(&netInfo_);
  memcpy(mac->addr, netInfo_.mac, 6);
}

void W5500Interface::get_IP(IpAddress *ip) const {
  wizchip_getnetinfo(&netInfo_);
  memcpy(ip->addr, netInfo_.ip, 4);
}

void W5500Interface::get_subnet(SubnetMask *subnet) const {
  wizchip_getnetinfo(&netInfo_);
  memcpy(subnet->addr, netInfo_.sn, 4);
}

void W5500Interface::get_gateway(GatewayAddress *gateway) const {
  wizchip_getnetinfo(&netInfo_);
  memcpy(gateway->addr, netInfo_.gw, 4);
}

}  // namespace Ethernet
