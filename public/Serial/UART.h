#ifndef PUBLIC_SERIAL_UART_H_
#define PUBLIC_SERIAL_UART_H_

#include <avr/io.h>
#include <avr/interrupt.h>

#include <cstdint>

#ifndef F_CPU
  #warning "F_CPU not defined for UART"
  #define F_CPU 16000000UL
#endif


namespace serial {
  enum class Communication_mode {
    kAsynchronous = (0 << UMSEL00) | (0 << UMSEL01),
    kSynchronous = (1 << UMSEL00) | (0 << UMSEL01),
    kMaster_spi = (1 << UMSEL00) | (1 << UMSEL01)
  };

  enum class Asynchronous_mode {
    kNormal = (0 << U2X0),
    kDouble_speed = (1 << U2X0)
  };

  enum class Baudrate {
    kBaud_2400,
    kBaud_4800,
    kBaud_9600,
    kBaud_14400,
    kBaud_19200,
    kBaud_38400,
    kBaud_57600,
    kBaud_76900,
    kBaud_115200,
  };

  enum class StopBits {
    kOne = (0 << USBS0),
    kTwo = (1 << USBS0)
  };

  enum class DataBits {
    kFive  = (0 << UCSZ00),
    kSix   = (1 << UCSZ00),
    kSeven = (2 << UCSZ00),
    kEight = (3 << UCSZ00),
  };

  enum class Parity {
    kNone = (0 << UPM00),
    kEven = (2 << UPM00),
    kOdd = (3 << UPM00)
  };

  enum class Serial_parameters {
    Communication_mode = Communication_mode::kAsynchronous,
    Asynchronous_mode = Asynchronous_mode::kNormal,
    Baudrate = Baudrate::kBaud_9600,
    Stop_bits = StopBits::kOne,
    Data_bits = DataBits::kEight,
    Parity_mode = Parity::kNone
  };

   
  class UART
  {
    public:
      volatile static uint8_t Tx_busy_{0};
      static constexpr const uint8_t is_busy{1};
      static constexpr const uint8_t is_not_busy{0};

      UART(const Serial_parameters &serial_parameters);
      void send_byte(const uint8_t byte);
      void send_bytes(const uint8_t const *bytes, const uint16_t lengths);
      void send_string(const char *string);

      ~UART() = default;

    private:
      constexpr const uint8_t kRX_buffer_size{16};
      constexpr const uint8_t kTX_buffer_size{16};
      static constexpr const uint8_t kAsynchronous_normal_speed_mode{16};
      static constexpr const uint8_t kAsynchronous_double_speed_mode{8};


    private:
      uint8_t Rx_buffer_[kRx_buffer_size_]{};
      volatile uint8_t *Rx_buffer_head_{serial_bufferRx};
      uint8_t Tx_buffer[kTx_buffer_size_]{};
      volatile uint8_t *Tx_buffer_head_{serial_bufferTx};


    private:
      static uint8_t calculate_baudrate_prescaler(const Baudrate &baudrate);
  };  // UART


}  // namespace serial
#endif  // PUBLIC_SERIAL_UART_H_