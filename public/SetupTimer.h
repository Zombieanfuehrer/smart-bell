#ifndef PUBLIC_SETUP_TIMER_INTERRUPT_H_
#define PUBLIC_SETUP_TIMER_INTERRUPT_H_

namespace timer_interrupt {
    enum class Prescaler : uint8_t {
      NONE = 0,
      DIV1 = (1 << CS00),
      DIV8 = (1 << CS01),
      DIV64 = (1 << CS01) | (1 << CS00),
      DIV256 = (1 << CS02),
      DIV1024 = (1 << CS02) | (1 << CS00)
  };

  namespace ctc_mode {
    void setup_timer0_ctc_mode(const Prescaler prescaler, const uint16_t ocr1a_value);
  }  // namespace ctc_mode

}  // namespace timer_interrupt

#endif  // PUBLIC_SETUP_TIMER_INTERRUPT_H_