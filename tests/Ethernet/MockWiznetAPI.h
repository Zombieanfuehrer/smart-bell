#ifndef TESTS_ETHERNET_MOCKWIZNETAPI_H_
#define TESTS_ETHERNET_MOCKWIZNETAPI_H_

#include <gmock/gmock.h>
#include <stdint.h>

// Forward-Deklarationen der benötigten Typen (um WIZnet-Header zu vermeiden)
typedef enum {
  CW_RESET_WIZCHIP = 0,
  CW_INIT_WIZCHIP,
  CW_GET_INTERRUPT,
  CW_CLR_INTERRUPT,
  CW_SET_INTRMASK,
  CW_GET_INTRMASK,
  CW_RESET_PHY,
  CW_SET_PHYCONF,
  CW_GET_PHYCONF,
  CW_GET_PHYSTATUS,
  CW_SET_PHYPOWMODE,
  CW_GET_PHYPOWMODE,
  CW_GET_PHYLINK
} ctlwizchip_type;

typedef enum {
  CS_SET_IOMODE,
  CS_GET_IOMODE,
  CS_GET_MAXTXBUF,
  CS_GET_MAXRXBUF,
  CS_CLR_INTERRUPT,
  CS_GET_INTERRUPT,
  CS_SET_INTMASK,
  CS_GET_INTMASK
} ctlsock_type;

typedef enum {
  SO_FLAG,
  SO_TTL,
  SO_TOS,
  SO_MSS,
  SO_DESTIP,
  SO_DESTPORT,
  SO_KEEPALIVEAUTO,
  SO_KEEPALIVESEND,
  SO_SENDBUF,
  SO_RECVBUF,
  SO_STATUS,
  SO_REMAINSIZE,
  SO_PACKINFO
} sockopt_type;

typedef struct {
  uint8_t mac[6];
  uint8_t ip[4];
  uint8_t sn[4];
  uint8_t gw[4];
  uint8_t dns[4];
  uint8_t dhcp;
} wiz_NetInfo;

// Socket-Konstanten
#ifndef SOCK_OK
#define SOCK_OK 1
#endif
#ifndef SOCK_CLOSED
#define SOCK_CLOSED 0
#endif
#ifndef SOCK_ERROR
#define SOCK_ERROR -1
#endif

// Socket-Status-Werte
#ifndef SOCK_INIT
#define SOCK_INIT 0x13
#endif
#ifndef SOCK_LISTEN
#define SOCK_LISTEN 0x14
#endif
#ifndef SOCK_ESTABLISHED
#define SOCK_ESTABLISHED 0x17
#endif
#ifndef SOCK_CLOSE_WAIT
#define SOCK_CLOSE_WAIT 0x1C
#endif
#ifndef SOCK_UDP
#define SOCK_UDP 0x22
#endif

// Mock für WIZnet Socket-API Funktionen
class MockWiznetAPI {
 public:
  virtual ~MockWiznetAPI() = default;

  // Socket-API Mocks
  MOCK_METHOD(int8_t, socket, (uint8_t sn, uint8_t protocol, uint16_t port, uint8_t flag));
  MOCK_METHOD(int8_t, close, (uint8_t sn));
  MOCK_METHOD(int8_t, listen, (uint8_t sn));
  MOCK_METHOD(int8_t, connect, (uint8_t sn, uint8_t* addr, uint16_t port));
  MOCK_METHOD(int8_t, disconnect, (uint8_t sn));
  MOCK_METHOD(int32_t, send, (uint8_t sn, uint8_t* buf, uint16_t len));
  MOCK_METHOD(int32_t, recv, (uint8_t sn, uint8_t* buf, uint16_t len));
  MOCK_METHOD(int32_t, sendto,
              (uint8_t sn, uint8_t* buf, uint16_t len, uint8_t* addr, uint16_t port));
  MOCK_METHOD(int32_t, recvfrom,
              (uint8_t sn, uint8_t* buf, uint16_t len, uint8_t* addr, uint16_t* port));
  MOCK_METHOD(int8_t, ctlsocket, (uint8_t sn, ctlsock_type cstype, void* arg));
  MOCK_METHOD(int8_t, setsockopt, (uint8_t sn, sockopt_type sotype, void* arg));
  MOCK_METHOD(int8_t, getsockopt, (uint8_t sn, sockopt_type sotype, void* arg));

  // W5500-Register Mocks
  MOCK_METHOD(uint8_t, getSn_SR, (uint8_t sn));
  MOCK_METHOD(uint16_t, getSn_RX_RSR, (uint8_t sn));
  MOCK_METHOD(uint16_t, getSn_TX_FSR, (uint8_t sn));
  MOCK_METHOD(void, setSn_IR, (uint8_t sn, uint8_t ir));
  MOCK_METHOD(uint8_t, getSn_IR, (uint8_t sn));

  // Wizchip-Konfiguration Mocks
  MOCK_METHOD(int8_t, ctlwizchip, (ctlwizchip_type cwtype, void* arg));
  MOCK_METHOD(void, reg_wizchip_cs_cbfunc, (void (*cs_sel)(), void (*cs_desel)()));
  MOCK_METHOD(void, reg_wizchip_spi_cbfunc, (uint8_t(*spi_rb)(), void (*spi_wb)(uint8_t)));
  MOCK_METHOD(void, reg_wizchip_spiburst_cbfunc,
              (void (*spi_rb)(uint8_t*, uint16_t), void (*spi_wb)(uint8_t*, uint16_t)));

  MOCK_METHOD(void, wizchip_setnetinfo, (wiz_NetInfo * pnetinfo));
  MOCK_METHOD(void, wizchip_getnetinfo, (wiz_NetInfo * pnetinfo));
};

// Globaler Mock-Zeiger für C-Funktions-Wrapper
extern MockWiznetAPI* g_mock_wiznet_api;

#endif  // TESTS_ETHERNET_MOCKWIZNETAPI_H_
