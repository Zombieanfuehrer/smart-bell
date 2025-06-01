#ifndef PUBLIC_ETHERNET_EMBEDDEDSOCKETINTERFACE_H_
#define PUBLIC_ETHERNET_EMBEDDEDSOCKETINTERFACE_H_

#include <stdint.h>

#include <Ethernet/W5500/w5500.h>

#include "W5500Interface.h"

namespace Ethernet {

enum class W5500SocketNumber : uint8_t {
  kSocket0 = 0,
  kSocket1 = 1,
  kSocket2 = 2,
  kSocket3 = 3,
  kSocket4 = 4,
  kSocket5 = 5,
  kSocket6 = 6,
  kSocket7 = 7
};

enum class W5500SocketFlag : uint16_t {
  kNone = 0x00,            // No flags set
  kEtherOwn = Sn_MR_MFEN,  // In MACRAW mode, receive only broadcast, multicast and own packets
  kIGMPv2 = Sn_MR_MC,      // In UDP with kMulticastEnable, select IGMP version 2
  kTCPNoDelay = Sn_MR_ND,  // In TCP mode, use no-delayed ACK
  kMulticastEnable = Sn_MR_MULTI,  // In UDP mode, enable multicast
  kBroadcastBlock = Sn_MR_BCASTB,  // In UDP or MACRAW mode, block broadcast packets
  kMulticastBlock = Sn_MR_MMB,     // In MACRAW mode, block multicast packets
  kIPv6Block = Sn_MR_MIP6B,        // In MACRAW mode, block IPv6 packets
  kUnicastBlock = Sn_MR_UCASTB     // In UDP with kMulticastEnable, block unicast packets
};

enum class W5500SocketProtocol : uint8_t {
  kTCP = 6,     // TCP protocol
  kUDP = 17,    // UDP protocol
  kIPRAW = 255  // IP raw mode
};

enum class W5500SocketState : uint16_t {
  kSOCK_CLOSED = 0x00,
  kSOCK_INIT = 0x13,
  kSOCK_LISTEN = 0x14,
  kSOCK_ESTABLISHED = 0x15,
  kSOCK_CLOSE_WAIT = 0x16,
  kSOCK_UDP = 0x22,
  kSOCK_MACRAW = 0x42,
  kSOCK_SYNSENT = 0x15,
  kSOCK_SYNRECV = 0x16,
  kSOCK_FIN_WAIT = 0x18,
  kSOCK_CLOSING = 0x1A,
  kSOCK_TIME_WAIT = 0x1B,
  kSOCK_LAST_ACK = 0x1D
};

struct W5500SocketStatus {
  W5500SocketNumber socket_number;    // Socket number
  W5500SocketProtocol protocol_type;  // Protocol type (TCP/UDP/IPRAW)
  uint16_t port;                      // Port number
  W5500SocketState current_state;     // Socket status
  W5500SocketFlag socket_flags;       // Socket flags
};

class EmbeddedSocketW5500 {
 public:
  EmbeddedSocketW5500(const Ethernet::W5500Interface* const w5500_interface);
  ~EmbeddedSocketW5500() = default;

  int8_t open_socket(W5500SocketStatus& socket_status);
  int8_t close_socket(uint8_t sn);
  int8_t disconnect_socket(uint8_t sn);
  int8_t listen_socket(uint8_t sn);
  int8_t connect_socket(uint8_t sn, const uint8_t* addr, uint16_t port);
  int32_t send_socket(uint8_t sn, const uint8_t* buf, uint16_t len);
  int32_t recv_socket(uint8_t sn, const uint8_t* buf, uint16_t len);
  int32_t sendto(uint8_t sn, const uint8_t* buf, uint16_t len, const uint8_t* addr, uint16_t port);
  int32_t recvfrom(uint8_t sn, const uint8_t* buf, uint16_t len, const uint8_t* addr,
                   const uint16_t* port);
  int8_t ctlsocket(uint8_t sn, ctlsock_type cstype, void* arg);
  int8_t setsockopt(uint8_t sn, sockopt_type sotype, void* arg);
  int8_t getsockopt(uint8_t sn, sockopt_type sotype, void* arg);

  W5500SocketStatus get_socket_status(uint8_t socket_num) const;

 private:
  const Ethernet::W5500Interface* const w5500_interface_ = nullptr;
  W5500SocketStatus socket_status_[8];

 private:
  uint8_t get_socket_index_(const W5500SocketNumber socket_num) const;
};

}  // namespace Ethernet

#endif  // PUBLIC_ETHERNET_EMBEDDEDSOCKETINTERFACE_H_
