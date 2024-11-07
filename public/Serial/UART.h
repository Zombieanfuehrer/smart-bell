#ifndef PUBLIC_SERIAL_UART_H_
#define PUBLIC_SERIAL_UART_H_

#include <avr/io.h>
#include <avr/interrupt.h>

#include <stdint.h>

#ifndef F_CPU
  #warning "F_CPU not defined for UART"
  #define F_CPU 16000000UL
#endif


namespace serial {
  enum class Communication_mode : uint8_t {
    kAsynchronous = (0 << UMSEL00) | (0 << UMSEL01),
    kSynchronous = (1 << UMSEL00) | (0 << UMSEL01),
    kMaster_spi = (1 << UMSEL00) | (1 << UMSEL01)
  };

  enum class Asynchronous_mode : uint8_t {
    kNormal = (0 << U2X0),
    kDouble_speed = (1 << U2X0)
  };

  enum class Baudrate : uint32_t {
    kBaud_2400 = 2400,
    kBaud_4800 = 4800,
    kBaud_9600 = 9600,
    kBaud_14400 = 14400,
    kBaud_19200 = 19200,
    kBaud_38400 = 38400,
    kBaud_57600 = 57600,
    kBaud_76900 = 76900,
    kBaud_115200 = 115200
  };

  enum class StopBits : uint8_t {
    kOne = (0 << USBS0),
    kTwo = (1 << USBS0)
  };

  enum class DataBits : uint8_t {
    kFive  = (0 << UCSZ00),
    kSix   = (1 << UCSZ00),
    kSeven = (2 << UCSZ00),
    kEight = (3 << UCSZ00),
  };

  enum class Parity : uint8_t {
    kNone = (0 << UPM00),
    kEven = (2 << UPM00),
    kOdd = (3 << UPM00)
  };

  struct Serial_parameters {
    Communication_mode communication_mode = Communication_mode::kAsynchronous;
    Asynchronous_mode asynchronous_mode = Asynchronous_mode::kNormal;
    Baudrate baudrate = Baudrate::kBaud_9600;
    StopBits stop_bits = StopBits::kOne;
    DataBits data_bits = DataBits::kEight;
    Parity parity_mode = Parity::kNone;
  };

   
  class UART
  {
    public:
      static constexpr const uint8_t is_busy{1};
      static constexpr const uint8_t is_not_busy{0};
      static constexpr const uint8_t kRX_buffer_size{16};
      static constexpr const uint8_t kTX_buffer_size{16};
      
      UART(const Serial_parameters &serial_parameters);

      void send_byte(const uint8_t byte);
      void send_bytes(const uint8_t *const bytes, const uint16_t lengths);
      void send_string(const char *string);

      uint16_t is_read_data_available() const;
      uint8_t read_byte();
      ~UART() = default;

    private:
      static constexpr const uint8_t kAsynchronous_normal_speed_mode{16};
      static constexpr const uint8_t kAsynchronous_double_speed_mode{8};

    private:

      uint8_t Tx_buffer_[UART::kTX_buffer_size] = {0};

    private:
      static uint8_t calculate_baudrate_prescaler(const Baudrate &baudrate, const Asynchronous_mode &asynchronous_mode);
  };  // UART


}  // namespace serial
#endif  // PUBLIC_SERIAL_UART_H_