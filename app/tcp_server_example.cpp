#ifdef __AVR__
#include <avr/io.h>
#include <util/delay.h>
#include <avr/wdt.h>
#endif

#include <Serial/SPI.h>
#include <Serial/UART.h>

#include <Ethernet/W5500/W5500Class.h>
#include <Ethernet/W5500/w5500.h>
#include <Ethernet/socket.h>

#define TCP_PORT 80        // HTTP-Port, kann nach Bedarf angepasst werden
#define SOCKET_NUMBER 0    // W5500 unterstützt 8 Sockets (0-7)
#define BUFFER_SIZE 2048   // Empfangspuffer

void run_tcp_server(Ethernet::W5500Interface* w5500) {
    uint8_t buffer[BUFFER_SIZE];
    int32_t received_size;
    
    // Socket initialisieren (TCP-Modus)
    socket(SOCKET_NUMBER, Sn_MR_TCP, TCP_PORT, 0);
    
    // Server-Socket in Listen-Modus versetzen
    if (listen(SOCKET_NUMBER) != SOCK_OK) {
        return; // Fehler beim Starten des Servers
    }
    
    while (1) {
        // Socket-Status prüfen
        uint8_t status = getSn_SR(SOCKET_NUMBER);
        
        switch(status) {
            case SOCK_ESTABLISHED: // Verbindung hergestellt
                // Daten empfangen, wenn verfügbar
                received_size = recv(SOCKET_NUMBER, buffer, BUFFER_SIZE);
                
                if (received_size > 0) {
                    // Echo zurücksenden
                    send(SOCKET_NUMBER, buffer, received_size);
                } 
                else if (received_size == SOCK_CLOSED) {
                    // Verbindung geschlossen
                    close(SOCKET_NUMBER);
                    // Neuen Socket öffnen
                    socket(SOCKET_NUMBER, Sn_MR_TCP, TCP_PORT, 0);
                    listen(SOCKET_NUMBER);
                }
                break;
                
            case SOCK_CLOSE_WAIT: // Verbindung wird geschlossen
                // Restliche Daten empfangen
                received_size = recv(SOCKET_NUMBER, buffer, BUFFER_SIZE);
                if (received_size > 0) {
                    send(SOCKET_NUMBER, buffer, received_size);
                }
                
                // Socket schließen
                close(SOCKET_NUMBER);
                // Neuen Socket öffnen und in Listen-Modus setzen
                socket(SOCKET_NUMBER, Sn_MR_TCP, TCP_PORT, 0);
                listen(SOCKET_NUMBER);
                break;
                
            case SOCK_CLOSED: // Socket geschlossen
                // Socket neu öffnen
                socket(SOCKET_NUMBER, Sn_MR_TCP, TCP_PORT, 0);
                listen(SOCKET_NUMBER);
                break;
                
            case SOCK_INIT: // Socket initialisiert
                listen(SOCKET_NUMBER);
                break;
                
            default:
                break;
        }
        
        _delay_ms(10); // Kurze Pause
    }
}

serial::SPI_parameters spi_params = {
    serial::SPI_mode::kMaster,
    serial::SPI_data_order::kMsb_first,
    serial::SPI_clock_polarity::kIdle_low,
    serial::SPI_clock_phase::kLeading,
    serial::SPI_clock_rate::k4mHz
};

serial::Serial_parameters uart_parms = {
  serial::Communication_mode::kAsynchronous,
  serial::Asynchronous_mode::kNormal,
  serial::Baudrate::kBaud_57600,
  serial::StopBits::kOne,
  serial::DataBits::kEight,
  serial::Parity::kNone
};

static const constexpr uint8_t kSPI_CS_W5500 = (1 << PORTB2);
static const constexpr uint8_t kRESET_W5500 = (1 << PORTD4);

int main() {
    wdt_disable();  // Watchdog deaktivieren
    sei(); 

    DDRD |= kSPI_CS_W5500;     // RST auf Pin 4 als Ausgang
    DDRB |= kRESET_W5500;      // CS auf Pin 10 als Ausgang

    serial::UART uart(uart_parms);
    uart.send_string("TCP Server Example\n\r");
    serial::SPI spi(spi_params, kSPI_CS_W5500); // Pin 10 als Slave Select (CS)
    
    Ethernet::W5500Callbacks callbacks = {
        .hard_reset = []() {
            PORTD &= ~kRESET_W5500;     // kRESET_W5500 auf LOW (aktiv)
            _delay_ms(1);
            PORTD |= kRESET_W5500;      // kRESET_W5500 auf HIGH
            _delay_ms(10);
        },
        .chip_select = []() {
            PORTB &= ~kSPI_CS_W5500;     // kSPI_CS_W5500 auf LOW (aktiv)
        },
        .chip_deselect = []() {
            PORTB |= kSPI_CS_W5500;      // kSPI_CS_W5500 auf HIGH
        }
    };
    
    // W5500 initialisieren
    Ethernet::W5500Interface w5500(&spi, callbacks, &uart);
    w5500.init();
    
    // Netzwerk konfigurieren
    Ethernet::MacAddress mac = {{0x00, 0x08, 0xDC, 0xAB, 0xCD, 0xEF}};
    Ethernet::IpAddress ip = {{192, 168, 1, 177}};
    Ethernet::SubnetMask subnet = {{255, 255, 255, 0}};
    Ethernet::GatewayAddress gateway = {{192, 168, 1, 1}};
    
    w5500.set_network_config(&mac, &ip, &subnet, &gateway);
    
    // TCP-Server starten
    run_tcp_server(&w5500);
    
    return 0;
}