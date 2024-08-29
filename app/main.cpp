
#include "SetupWDT.h"
#include "SetupEXT_IN_Interrupt.h"

int main () {
    configure_wdt::timeout_1sec_reset_power::setup_WTD();
    external_pin_interrupt::setup_INT0_PullUpResistorFallingEdge();

    while (1) {
        // Do nothing
    }
    return 0;
}