#ifndef PUBLIC_SETUP_EXT_IN_INTERRUPT_H_
#define PUBLIC_SETUP_EXT_IN_INTERRUPT_H_

#include <stdint.h>

namespace external_pin_interrupt {

/// Interrupt source identifiers
enum class InterruptSource : uint8_t {
  kINT0 = 0,  ///< PD2 - Frontdoor button
  kINT1 = 1   ///< PD3 - Office button
};

/// Edge type for interrupt
enum class EdgeType : uint8_t {
  kFalling = 0,  ///< Trigger on falling edge only
  kRising = 1,   ///< Trigger on rising edge only
  kAnyChange = 2 ///< Trigger on any change (both edges)
};

/**
 * @brief Setup INT0 (PD2) with pull-up and specified edge detection.
 * @param edge Edge type for interrupt triggering.
 */
void setup_INT0(EdgeType edge = EdgeType::kAnyChange);

/**
 * @brief Setup INT1 (PD3) with pull-up and specified edge detection.
 * @param edge Edge type for interrupt triggering.
 */
void setup_INT1(EdgeType edge = EdgeType::kAnyChange);

/**
 * @brief Setup both INT0 and INT1 with pull-up and specified edge detection.
 * @param int0_edge Edge type for INT0.
 * @param int1_edge Edge type for INT1.
 */
void setup_both_interrupts(EdgeType int0_edge = EdgeType::kAnyChange,
                           EdgeType int1_edge = EdgeType::kAnyChange);

/**
 * @brief Read current state of INT0 pin (PD2).
 * @return true if HIGH, false if LOW.
 */
bool read_INT0_state();

/**
 * @brief Read current state of INT1 pin (PD3).
 * @return true if HIGH, false if LOW.
 */
bool read_INT1_state();

// Legacy function for backwards compatibility
void setup_INT0_PullUpResistorFallingEdge(void);

}  // namespace external_pin_interrupt

#endif  // PUBLIC_SETUP_EXT_IN_INTERRUPT_H_