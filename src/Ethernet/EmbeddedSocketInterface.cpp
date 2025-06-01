#include <stdint.h>

#include <Ethernet/W5500/w5500.h>
#include <Ethernet/socket.h>

#include "EmbeddedSocketInterface.h"
#include "W5500Interface.h"

namespace Ethernet {

EmbeddedSocketW5500::EmbeddedSocketW5500(const Ethernet::W5500Interface* const w5500_interface)
    : w5500_interface_(w5500_interface) {}

int8_t EmbeddedSocketW5500::open_socket(W5500SocketStatus& socket_status) {
  uint8_t socket_idx = get_socket_index_(socket_status.socket_number);
  socket_status_[socket_idx] = socket_status;

  auto ret_val = socket(socket_idx, static_cast<uint8_t>(socket_status.protocol_type),
                        socket_status.port, static_cast<uint8_t>(socket_status.socket_flags));

  if (ret_val == SOCK_OK) {
    socket_status_[socket_idx].current_state = static_cast<W5500SocketState>(getSn_SR(socket_idx));
  } else {
    socket_status_[socket_idx].current_state = W5500SocketState::kSOCK_CLOSED;
  }
  return ret_val;
}

int8_t EmbeddedSocketW5500::close_socket(uint8_t sn) { return close(sn); }

int8_t EmbeddedSocketW5500::disconnect_socket(uint8_t sn) { return disconnect(sn); }

int8_t EmbeddedSocketW5500::listen_socket(uint8_t sn) { return listen(sn); }

int8_t Ethernet::EmbeddedSocketW5500::connect_socket(uint8_t sn, const uint8_t* addr,
                                                     uint16_t port) {
  return connect(sn, const_cast<uint8_t*>(addr), port);
}

int32_t Ethernet::EmbeddedSocketW5500::send_socket(uint8_t sn, const uint8_t* buf, uint16_t len) {
  return send(sn, const_cast<uint8_t*>(buf), len);
}

int32_t Ethernet::EmbeddedSocketW5500::recv_socket(uint8_t sn, const uint8_t* buf, uint16_t len) {
  return recv(sn, const_cast<uint8_t*>(buf), len);
}

int32_t Ethernet::EmbeddedSocketW5500::sendto(uint8_t sn, const uint8_t* buf, uint16_t len,
                                              const uint8_t* addr, uint16_t port) {
  return sendto(sn, const_cast<uint8_t*>(buf), len, const_cast<uint8_t*>(addr), port);
}

int32_t Ethernet::EmbeddedSocketW5500::recvfrom(uint8_t sn, const uint8_t* buf, uint16_t len,
                                                const uint8_t* addr, const uint16_t* port) {
  return recvfrom(sn, const_cast<uint8_t*>(buf), len, const_cast<uint8_t*>(addr),
                  const_cast<uint16_t*>(port));
}

int8_t Ethernet::EmbeddedSocketW5500::ctlsocket(uint8_t sn, ctlsock_type cstype, void* arg) {
  return ctlsocket(sn, cstype, arg);
}

int8_t Ethernet::EmbeddedSocketW5500::setsockopt(uint8_t sn, sockopt_type sotype, void* arg) {
  return setsockopt(sn, sotype, arg);
}

int8_t Ethernet::EmbeddedSocketW5500::getsockopt(uint8_t sn, sockopt_type sotype, void* arg) {
  return getsockopt(sn, sotype, arg);
}

W5500SocketStatus EmbeddedSocketW5500::get_socket_status(uint8_t socket_num) const {
  if (socket_num >= 8) {
    static W5500SocketStatus invalid_status = {};
    return invalid_status;
  }
  return socket_status_[socket_num];
}

uint8_t EmbeddedSocketW5500::get_socket_index_(const W5500SocketNumber socket_num) const {
  return static_cast<uint8_t>(socket_num);
}
}  // namespace Ethernet