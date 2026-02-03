# Smart Bell MQTT Implementation

This implementation adds MQTT connectivity to the Smart Bell project, allowing remote monitoring and control via MQTT protocol.

## Features

### Core Functionality
- **MQTT Client**: Full MQTT 3.1.1 client implementation with QoS 0 support
- **Button Press Detection**: Publishes MQTT message when doorbell button is pressed
- **Remote Control**: Subscribe to control topic for enable/disable commands
- **Automatic Reconnection**: 30-second reconnect interval on connection loss
- **UART Configuration**: Configure MQTT credentials and settings via UART terminal

### Network Features
- **W5500 Ethernet**: Uses W5500 Ethernet controller via SPI
- **Static IP**: Configurable static IP addressing (DHCP integration pending)
- **MAC Address**: Configurable MAC address

### Configuration Management
- **EEPROM Storage**: Save configuration persistently
- **UART Interface**: Interactive configuration via serial terminal
- **Default Settings**: Sensible defaults for quick setup

## Hardware Requirements

- **Microcontroller**: ATmega328P (Arduino Uno compatible)
- **Ethernet**: W5500 Ethernet module connected via SPI
- **Button**: Connected to INT0 (PD2) with pull-up resistor
- **Output**: Bell activation on PB0

### Pin Connections

```
ATmega328P Pin Assignments:
- PD2 (INT0): Doorbell button input (with pull-up, triggers on falling edge)
- PB0: Bell/Ring output
- PB2: W5500 SPI CS (Chip Select)
- PD4: W5500 Reset
- PB3 (MOSI): SPI MOSI to W5500
- PB4 (MISO): SPI MISO from W5500
- PB5 (SCK): SPI Clock to W5500
- PD0 (RX): UART RX for configuration
- PD1 (TX): UART TX for debugging
```

## Software Architecture

### Modules

1. **MQTT Client** (`public/MQTT/MQTTClient.h`, `src/MQTT/MQTTClient.cpp`)
   - MQTT 3.1.1 protocol implementation
   - Connection management with keepalive
   - Publish/Subscribe functionality
   - Automatic reconnection

2. **Configuration Manager** (`public/Config/ConfigManager.h`, `src/Config/ConfigManager.cpp`)
   - EEPROM persistence
   - UART command parser
   - Network and MQTT settings management

3. **Smart Bell Application** (`app/smart_bell.cpp`)
   - Main firmware integrating all components
   - Button interrupt handling
   - MQTT event processing
   - Watchdog timer management

## Building the Firmware

### Prerequisites

- AVR-GCC toolchain (8-bit)
- CMake 3.20+
- Conan 2.0+
- Python 3 with pip

### Build Steps

```bash
# Install dependencies
conan install . --build=missing -pr:h=avr-mega328p_g -o W5500_support=True

# Source environment
. build/Debug/generators/conanbuild.sh

# Configure CMake
cmake --preset conan-generated-avr-debug

# Build
cmake --build build/Debug --target ATmega328_SMART_BELL_FW_hex
```

The firmware hex file will be located at: `build/Debug/app/ATmega328_SMART_BELL_FW.hex`

## Configuration

### UART Commands

Connect to the UART at 19200 baud, 8N1. Available commands:

```
config show                           - Show current configuration
config save                           - Save configuration to EEPROM
config load                           - Load configuration from EEPROM  
config defaults                       - Load default configuration
set broker <host> <port>              - Set MQTT broker (e.g., "set broker 192.168.1.100 1883")
set cred <id> <user> <pass>           - Set MQTT credentials
set topics <ring> <control>           - Set MQTT topics
set dhcp <on|off>                     - Enable/disable DHCP (currently only 'off' works)
set ip <ip> <subnet> <gateway>        - Set static IP configuration
help                                  - Show command help
```

### Default Configuration

```
MQTT Broker: 192.168.1.100:1883
Client ID: smart-bell
Username: (none)
Password: (none)
Ring Topic: home/bell/ring
Control Topic: home/bell/control
Network: DHCP (fallback to 192.168.1.177/24)
Ring Duration: 6 seconds
Reconnect Interval: 30 seconds
Watchdog Timeout: 4 seconds
```

### Example Configuration Session

```
> set broker mqtt.local 1883
[Config] Broker set to: mqtt.local

> set cred doorbell myuser mypassword
[Config] Credentials updated

> set topics home/doorbell/pressed home/doorbell/control
[Config] Topics updated

> set ip 192.168.1.100 255.255.255.0 192.168.1.1
[Config] Static IP configured

> config save
[Config] Configuration saved to EEPROM

> config show
=== Smart Bell Configuration ===
MQTT Broker: mqtt.local:1883
Client ID: doorbell
Username: myuser
Password: ********
Ring Topic: home/doorbell/pressed
Control Topic: home/doorbell/control
DHCP: enabled
Ring Duration: 6 seconds
Reconnect Interval: 30 seconds
===============================
```

## MQTT Topics

### Published Topics

**Ring Event** (default: `home/bell/ring`)
- Payload: `"RING"`
- Published when doorbell button is pressed
- QoS: 0
- Retain: false

### Subscribed Topics

**Control Commands** (default: `home/bell/control`)
- `"enable"`: Enable the bell (allows ringing)
- `"disable"`: Disable the bell (silences ringing)
- `"status"`: Request status (future feature)
- QoS: 0

## Usage Example

### Home Assistant Integration

```yaml
# configuration.yaml
mqtt:
  sensor:
    - name: "Doorbell"
      state_topic: "home/bell/ring"
      value_template: "{{ value }}"
      
  switch:
    - name: "Doorbell Enable"
      command_topic: "home/bell/control"
      payload_on: "enable"
      payload_off: "disable"
      state_topic: "home/bell/status"
      optimistic: true
```

### Node-RED Example

```javascript
// Listen for doorbell rings
msg.topic = "home/bell/ring";
// When received, trigger notification

// Disable bell for 1 hour
msg.topic = "home/bell/control";
msg.payload = "disable";
// After 1 hour, send "enable"
```

## Debug Output

The firmware provides extensive UART debug output:

```
=====================================
 Smart Bell with MQTT - Version 1.0
=====================================

[Config] Loaded default configuration
[System] Using default configuration
=== Smart Bell Configuration ===
...
[System] Type 'help' for available commands

[Ethernet] Initializing W5500...
[Ethernet] W5500 initialized
[Ethernet] Configuring network...
[Ethernet] Network configured
[MQTT] Connecting to broker...
[MQTT] Connected to broker
[MQTT] Subscribed to topic: home/bell/control
[System] Smart Bell started

[Bell] Button pressed - publishing MQTT message
[MQTT] Published to topic: home/bell/ring
[Bell] Ring completed
```

## Troubleshooting

### MQTT Connection Issues

1. **Check network configuration**:
   ```
   > config show
   ```
   Verify IP, subnet, gateway are correct

2. **Verify broker is reachable**:
   - Ping the broker from another device on the network
   - Check broker logs for connection attempts

3. **Check MQTT credentials**:
   - Ensure username/password are correct
   - Some brokers require authentication even if empty

### No Ring on Button Press

1. **Check bell enabled status**:
   Send `enable` command to control topic

2. **Verify button connection**:
   Check debug output for "Button pressed" message

3. **Check MQTT connection**:
   Ensure "[MQTT] Connected to broker" appears in output

### Configuration Not Saving

- EEPROM operations only work on real hardware
- On simulator/emulator, config is lost on reset
- Use `config save` command to persist settings

## Testing

Unit tests are provided for hardware-independent components:

```bash
# Build with tests enabled
conan install . --build=missing -pr:h=default -o platform=linux -o tests=True

# Run tests
cmake --build build/linux --target run_gTest
./build/linux/run_gTest
```

## Future Enhancements

- [ ] DHCP client integration
- [ ] DNS resolution for broker hostname
- [ ] TLS/SSL support (if memory allows)
- [ ] QoS 1/2 support
- [ ] Status reporting
- [ ] Multiple bell patterns
- [ ] Web configuration interface

## License

MIT License - See LICENSE file for details

## Credits

- WIZnet ioLibrary for W5500 driver
- MQTT protocol specification: OASIS MQTT Version 3.1.1
