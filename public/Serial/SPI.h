#ifndef PUBLIC_SERIAL_SPI_H_
#define PUBLIC_SERIAL_SPI_H_

#ifdef __AVR__
#include <avr/io.h>
#endif
#include "Serial/Interface.h"
#include "Utils/CircularBuffer.h"

#ifndef F_CPU
  #warning "F_CPU not defined for UART"
  #define F_CPU 16000000UL
#endif

namespace serial
{
  enum class SPI_mode : uint8_t
  {
    kMaster = (1 << MSTR),
    kSlave = (0 << MSTR)
  };

  enum class SPI_data_order : uint8_t
  {
    kMsb_first = (0 << DORD),
    kLsb_first = (1 << DORD)
  };

  enum class SPI_clock_polarity : uint8_t
  {
    kIdle_low = (0 << CPOL),
    kIdle_high = (1 << CPOL)
  };

  enum class SPI_clock_phase : uint8_t
  {
    kLeading = (0 << CPHA),
    kTrailing = (1 << CPHA)
  };

  enum class SPI_clock_rate : uint8_t
  {
    k8mHz = (1 << SPI2X) | (0 << SPR1) | (0 << SPR0),
    k4mHz = (0 << SPR1) | (0 << SPR0),
    k2mHz = (1 << SPI2X) | (0 << SPR1) | (1 << SPR0),
    k1mHz = (0 << SPR1) | (1 << SPR0),
    k500kHz = (1 << SPI2X) | (1 << SPR1) | (1 << SPR0),
    k250kHz = (1 << SPR1) | (0 << SPR0),
    k125kHz = (1 << SPR1) | (1 << SPR0)
  };

  struct SPI_parameters
  {
    SPI_mode spi_mode = SPI_mode::kMaster;
    SPI_data_order data_order = SPI_data_order::kMsb_first;
    SPI_clock_polarity clock_polarity = SPI_clock_polarity::kIdle_low;
    SPI_clock_phase clock_phase = SPI_clock_phase::kLeading;
    SPI_clock_rate clock_rate = SPI_clock_rate::k4mHz;
  };

  class SPI : public Interface
  {
   public:
    SPI(const SPI_parameters &parameters, const uint8_t slave_select);
    ~SPI() = default;
    static constexpr const uint8_t kTXRX_buffer_size{250};

    void set_slave_select(const uint8_t slave_select);
    void send(const uint8_t byte) override;
    void send_bytes(const uint8_t *const bytes, const uint16_t length) override;
    void send_string(const char *string) override;
    bool is_read_data_available() const override;
    uint8_t read_byte() override;

   public:
    static const constexpr uint8_t kMOSI = (1 << DDB3);
    static const constexpr uint8_t kMISO = (0 << DDB4);
    static const constexpr uint8_t kSCK = (1 << DDB5);

    private:
    void send_();
    uint8_t slave_select_;
    
  };

} // namespace serial

#endif // PUBLIC_SERIAL_SPI_H_