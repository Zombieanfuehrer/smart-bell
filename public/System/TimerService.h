#ifndef PUBLIC_SYSTEM_TIMERSERVICE_H_
#define PUBLIC_SYSTEM_TIMERSERVICE_H_

#include <stdint.h>

namespace System {

/**
 * @brief Timer service for providing millisecond and second ticks.
 *
 * This service provides timing functionality for DHCP, DNS, and MQTT clients.
 * The tick handlers must be called from appropriate timer ISRs:
 * - on_1ms_tick(): Call from a 1ms timer interrupt (Timer0)
 * - on_1s_tick(): Call from a 1 second timer interrupt (Timer1)
 */
class TimerService {
 public:
  TimerService() = default;
  ~TimerService() = default;

  /**
   * @brief Call this from a 1ms timer ISR.
   * Increments millisecond counter and calls MQTT MilliTimer_Handler().
   */
  static void on_1ms_tick();

  /**
   * @brief Call this from a 1 second timer ISR.
   * Increments second counter and calls DHCP_time_handler(), DNS_time_handler().
   */
  static void on_1s_tick();

  /**
   * @brief Get elapsed milliseconds since startup (wraps at ~49 days).
   * @return Milliseconds counter value.
   */
  static uint32_t millis();

  /**
   * @brief Get elapsed seconds since startup (wraps at ~136 years).
   * @return Seconds counter value.
   */
  static uint32_t seconds();

  /**
   * @brief Check if a timeout has elapsed since a start time.
   * @param start_time Start time in milliseconds (from millis()).
   * @param timeout_ms Timeout duration in milliseconds.
   * @return true if timeout has elapsed.
   */
  static bool has_elapsed_ms(uint32_t start_time, uint32_t timeout_ms);

  /**
   * @brief Check if a timeout has elapsed since a start time.
   * @param start_time Start time in seconds (from seconds()).
   * @param timeout_sec Timeout duration in seconds.
   * @return true if timeout has elapsed.
   */
  static bool has_elapsed_sec(uint32_t start_time, uint32_t timeout_sec);

  /**
   * @brief Reset all counters to zero.
   */
  static void reset();

 private:
  static volatile uint32_t millis_counter_;
  static volatile uint32_t seconds_counter_;
};

}  // namespace System

#endif  // PUBLIC_SYSTEM_TIMERSERVICE_H_
