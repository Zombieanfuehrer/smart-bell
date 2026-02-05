#ifndef PUBLIC_SETUP_WDT_H_
#define PUBLIC_SETUP_WDT_H_

namespace configure_wdt {
namespace timeout_1sec_reset_power {
void setup_WDT(void);
}  // namespace timeout_1sec_reset_power

namespace timeout_8sec_reset_power {
void setup_WDT(void);
}  // namespace timeout_8sec_reset_power

// WDT control functions for long operations (EEPROM, network)
void pause(void);    // Disable WDT temporarily
void resume(void);   // Re-enable WDT with 8s timeout
void reset(void);    // Reset WDT counter
void disable(void);  // Completely disable WDT
}  // namespace configure_wdt

#endif  // PUBLIC_SETUP_WDT_H_