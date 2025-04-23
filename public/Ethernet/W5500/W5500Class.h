#ifndef PUBLIC_ETERNET_W5500_W5500CLASS_H_
#define PUBLIC_ETHERNET_W5500_W5500CLASS_H_

#include "Serial/SPI.h"
#include "Ethernet/wizchip_conf.h"

class W5500Class
{
public:
    W5500Class(serial::SPI *SPI);
    ~W5500Class();

private:
    serial::SPI *spi_ = nullptr;

};



#endif // PUBLIC_ETHERNET_W5500_W5500CLASS_H_


