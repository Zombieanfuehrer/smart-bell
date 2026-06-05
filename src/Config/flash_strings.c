#include <avr/pgmspace.h>

// Reine C-Definition im Flash
const char smart_bell_flash_help[] __attribute__((__progmem__)) =
    "Commands:\r\n"
    "  ip <a.b.c.d>        - Set device IP\r\n"
    "  sn <a.b.c.d>        - Set subnet mask\r\n"
    "  gw <a.b.c.d>        - Set gateway\r\n"
    "  br <a.b.c.d>:<port> - Set broker IP and port\r\n"
    "  id <name>           - Set MQTT client ID\r\n"
    "  input 1 pt <name>   - Set publish topic for Bell Button 1\r\n"
    "  input 2 pt <name>   - Set publish topic for Bell Button 2\r\n"
    "  gong sub <name>     - Base topic Chime 1&2. Ex: <name>/1 (CMD: ON/OFF/RING)\r\n"
    "  show                - Show current configuration\r\n"
    "  save                - Save configuration to EEPROM\r\n"
    "  reset               - Load factory defaults\r\n"
    "  reboot              - Restart the microcontroller\r\n";
