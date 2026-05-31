#ifndef PUBLIC_APP_GONGCONTROLLER_H_
#define PUBLIC_APP_GONGCONTROLLER_H_

#include <stdint.h>

namespace App {

/// Gong output identifiers
enum class GongId : uint8_t {
  kUpperfloor = 0,   ///< PD4 output
  kGroundfloor = 1,  ///< PD5 output
  kBoth = 2          ///< Both outputs
};

/**
 * @brief Controller for gong relay outputs.
 *
 * Manages two gong outputs (PD4 for upperfloor, PD5 for groundfloor).
 * Each output can be triggered for a configurable duration.
 * Outputs are HIGH-active (HIGH = gong ringing).
 */
class GongController {
 public:
  /// Default gong duration in milliseconds
  static constexpr uint16_t kDefaultDurationMs = 2000;

  /// Minimum gong duration
  static constexpr uint16_t kMinDurationMs = 100;

  /// Maximum gong duration
  static constexpr uint16_t kMaxDurationMs = 10000;

  GongController();
  ~GongController() = default;

  /**
   * @brief Initialize GPIO pins for gong outputs.
   * Sets PD4 and PD5 as outputs, initially LOW.
   */
  void init();

  /**
   * @brief Process gong timers. Call this in main loop.
   * Turns off gongs after their duration expires.
   */
  void update();

  /**
   * @brief Trigger a gong output.
   * @param gong Which gong to trigger.
   * @param force If true, trigger even if gong is disabled.
   * @param duration_ms Duration in ms (0 = use default).
   */
  void trigger(GongId gong, bool force = false, uint16_t duration_ms = 0);

  /**
   * @brief Set enable status for a gong.
   * @param gong Which gong (kUpperfloor or kGroundfloor only).
   * @param enabled true to enable, false to disable.
   */
  void set_enabled(GongId gong, bool enabled);

  /**
   * @brief Check if a gong is enabled.
   * @param gong Which gong.
   * @return true if enabled.
   */
  bool is_enabled(GongId gong) const;

  /**
   * @brief Check if a gong is currently active (ringing).
   * @param gong Which gong.
   * @return true if currently ringing.
   */
  bool is_active(GongId gong) const;

  /**
   * @brief Set default gong duration.
   * @param duration_ms Duration in milliseconds.
   */
  void set_duration(uint16_t duration_ms);

  /**
   * @brief Get current default gong duration.
   * @return Duration in milliseconds.
   */
  uint16_t get_duration() const;

  /**
   * @brief Trigger enabled gongs based on button press.
   * Only triggers gongs that are enabled.
   */
  void trigger_enabled_gongs();

 private:
  /// Gong state structure
  struct GongState {
    bool enabled;          ///< Whether this gong responds to buttons
    bool active;           ///< Currently ringing
    uint32_t stop_time;    ///< Millisecond timestamp when to stop
  };

  GongState upperfloor_;
  GongState groundfloor_;
  uint16_t default_duration_ms_;

  /**
   * @brief Set a gong output HIGH or LOW.
   * @param gong Which gong.
   * @param state true = HIGH, false = LOW.
   */
  void set_output(GongId gong, bool state);
};

}  // namespace App

#endif  // PUBLIC_APP_GONGCONTROLLER_H_
