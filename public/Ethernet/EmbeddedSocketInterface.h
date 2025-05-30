#ifndef PUBLIC_ETHERNET_EMBEDDEDSOCKETINTERFACE_H_
#define PUBLIC_ETHERNET_EMBEDDEDSOCKETINTERFACE_H_

#include "W5500Interface.h"

namespace Ethernet {

class EmbeddedSocketW5500 {
 public:
  EmbeddedSocketW5500(const Ethernet::W5500Interface* const w5500_interface);
  ~EmbeddedSocketW5500() = default;

 private:
  const Ethernet::W5500Interface* const w5500_interface_ = nullptr;
};

}  // namespace Ethernet

#endif  // PUBLIC_ETHERNET_EMBEDDEDSOCKETINTERFACE_H_
