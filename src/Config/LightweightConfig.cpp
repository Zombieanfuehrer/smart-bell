// Lightweight ConfigManager - Optimized for minimal SRAM usage
// Uses manual string parsing to avoid vfprintf_std/vfscanf_std (~800B)

#include "Config/LightweightConfig.h"

#include <stddef.h>  // for size_t
#include <string.h>

// Platform-specific includes
#ifdef __AVR__
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#else
// Test/Linux build - use mocks
#include "../../tests/Mocks/AVRHardwareMock.h"
#ifndef PROGMEM
#define PROGMEM
#endif
#ifndef PSTR
#define PSTR(s) (s)
#endif
#endif
#ifdef __AVR__
#include "Serial/UART.h"
#else
// For tests, use forward declaration only (mock provides interface)
#include "Serial/Interface.h"
#endif

namespace Config {

constexpr uint16_t EEPROM_ADDR = 128;
constexpr uint16_t EEPROM_ADDR_LEGACY = 0;
constexpr uint32_t MAGIC = 0xBEEFCAFE;

inline bool is_valid_config(const SmartBellConfig& cfg) {
  if (cfg.magic != MAGIC) {
    return false;
  }
  if (cfg.broker_port == 0) {
    return false;
  }
  return true;
}

void eeprom_read_cfg(SmartBellConfig* dst, uint16_t addr) {
#ifdef __AVR__
  eeprom_read_block(dst, reinterpret_cast<void*>(addr), sizeof(SmartBellConfig));
#else
  eeprom_read_block(dst, (void*)(uintptr_t)addr, sizeof(SmartBellConfig));
#endif
}

void eeprom_write_cfg(const SmartBellConfig* src, uint16_t addr) {
#ifdef __AVR__
  eeprom_update_block(src, reinterpret_cast<void*>(addr), sizeof(SmartBellConfig));
#else
  eeprom_update_block(src, (void*)(uintptr_t)addr, sizeof(SmartBellConfig));
#endif
}

inline bool parse_port_u16(const char* str, uint16_t* out_port) {
  if (!str || !out_port) {
    return false;
  }

  while (*str == ' ') {
    str++;
  }

  if (*str < '0' || *str > '9') {
    return false;
  }

  uint32_t port = 0;
  while (*str >= '0' && *str <= '9') {
    port = (port * 10U) + static_cast<uint32_t>(*str - '0');
    if (port > 65535U) {
      return false;
    }
    str++;
  }

  while (*str == ' ') {
    str++;
  }

  if (*str != '\0' || port == 0U) {
    return false;
  }

  *out_port = static_cast<uint16_t>(port);
  return true;
}

// Help text in PROGMEM (saves SRAM)
const char HELP_TEXT[] PROGMEM =
    "Commands:\r\n"
    "  ip <a.b.c.d>    - Set device IP\r\n"
    "  gw <a.b.c.d>    - Set gateway\r\n"
    "  br <a.b.c.d>:<port> - Set broker\r\n"
    "  id <name>       - Set client ID\r\n"
    "  show            - Show config\r\n"
    "  save            - Save to EEPROM\r\n"
    "  reset           - Load defaults\r\n";

LightweightConfig::LightweightConfig(serial::Interface& uart) : uart_(&uart) {
  reset_to_defaults();
}

void LightweightConfig::reset_to_defaults() {
  // MQTT defaults - all zero = not configured (user must set with 'br' command)
  config_.broker_ip[0] = 0;
  config_.broker_ip[1] = 0;
  config_.broker_ip[2] = 0;
  config_.broker_ip[3] = 0;
  config_.broker_port = 1883;
  strncpy(config_.client_id, "smartbell", 15);
  config_.client_id[15] = '\0';

  // Network defaults
  config_.device_ip[0] = 192;
  config_.device_ip[1] = 168;
  config_.device_ip[2] = 1;
  config_.device_ip[3] = 100;

  config_.subnet[0] = 255;
  config_.subnet[1] = 255;
  config_.subnet[2] = 255;
  config_.subnet[3] = 0;

  config_.gateway[0] = 192;
  config_.gateway[1] = 168;
  config_.gateway[2] = 1;
  config_.gateway[3] = 1;

  config_.mac[0] = 0x00;
  config_.mac[1] = 0x08;
  config_.mac[2] = 0xDC;
  config_.mac[3] = 0xAB;
  config_.mac[4] = 0xCD;
  config_.mac[5] = 0xEF;

  config_.magic = MAGIC;
}

void LightweightConfig::load() {
  SmartBellConfig temp;

  eeprom_read_cfg(&temp, EEPROM_ADDR);
  if (is_valid_config(temp)) {
    memcpy(&config_, &temp, sizeof(SmartBellConfig));
    msg("[CFG] Loaded\r\n");
    return;
  }

  // One-time migration path for older firmware using EEPROM address 0.
  eeprom_read_cfg(&temp, EEPROM_ADDR_LEGACY);
  if (is_valid_config(temp)) {
    memcpy(&config_, &temp, sizeof(SmartBellConfig));
    eeprom_write_cfg(&config_, EEPROM_ADDR);
    msg("[CFG] Loaded (migrated)\r\n");
    return;
  }

  msg("[CFG] Invalid, using defaults\r\n");
}

void LightweightConfig::save() {
  config_.magic = MAGIC;
  eeprom_write_cfg(&config_, EEPROM_ADDR);
  save_flag_ = true;
  msg("[CFG] Saved\r\n");
}

// Manual IP parser: "192.168.1.100" → {192,168,1,100}
// Avoids sscanf to save ~800B SRAM
bool LightweightConfig::parse_ip(const char* str, uint8_t* ip) {
  if (!str || !ip)
    return false;

  uint8_t octet = 0;
  uint16_t val = 0;

  while (*str) {
    if (*str >= '0' && *str <= '9') {
      val = val * 10 + (*str - '0');
      if (val > 255)
        return false;
    } else if (*str == '.') {
      if (octet >= 3)
        return false;
      ip[octet++] = (uint8_t)val;
      val = 0;
    } else {
      // Stop parsing at non-IP characters (space, newline, etc.)
      break;
    }
    str++;
  }

  if (octet != 3)
    return false;
  ip[3] = (uint8_t)val;
  return true;
}

void LightweightConfig::msg(const char* text) {
  if (uart_) {
    uart_->send_string(text);
  }
}

// Simple command parser - no complex logic
bool LightweightConfig::process_command(const char* cmd) {
  if (!cmd || !uart_)
    return false;

  // Skip leading whitespace
  while (*cmd == ' ')
    cmd++;

  // "help"
  if (strncmp(cmd, "help", 4) == 0) {
#ifdef __AVR__
    // Send PROGMEM string byte-by-byte - avoids stack buffer entirely
    const char* p = HELP_TEXT;
    char ch;
    while ((ch = pgm_read_byte(p++)) != '\0') {
      uart_->send(static_cast<uint8_t>(ch));
    }
#else
    uart_->send_string(HELP_TEXT);
#endif
    return true;
  }

  // "show"
  if (strncmp(cmd, "show", 4) == 0) {
    msg("Device IP: ");
    // Manual IP printing to avoid sprintf (~400B)
    char buf[4];
    for (int i = 0; i < 4; i++) {
      uint8_t val = config_.device_ip[i];
      uint8_t idx = 0;
      if (val >= 100) {
        buf[idx++] = '0' + val / 100;
        val %= 100;
        buf[idx++] = '0' + val / 10;
        val %= 10;
      } else if (val >= 10) {
        buf[idx++] = '0' + val / 10;
        val %= 10;
      }
      buf[idx++] = '0' + val;
      buf[idx] = '\0';
      uart_->send_string(buf);
      if (i < 3)
        uart_->send_string(".");
    }
    msg("\r\nGateway: ");
    for (int i = 0; i < 4; i++) {
      uint8_t val = config_.gateway[i];
      uint8_t idx = 0;
      if (val >= 100) {
        buf[idx++] = '0' + val / 100;
        val %= 100;
        buf[idx++] = '0' + val / 10;
        val %= 10;
      } else if (val >= 10) {
        buf[idx++] = '0' + val / 10;
        val %= 10;
      }
      buf[idx++] = '0' + val;
      buf[idx] = '\0';
      uart_->send_string(buf);
      if (i < 3)
        uart_->send_string(".");
    }
    msg("\r\nBroker: ");
    for (int i = 0; i < 4; i++) {
      uint8_t val = config_.broker_ip[i];
      uint8_t idx = 0;
      if (val >= 100) {
        buf[idx++] = '0' + val / 100;
        val %= 100;
        buf[idx++] = '0' + val / 10;
        val %= 10;
      } else if (val >= 10) {
        buf[idx++] = '0' + val / 10;
        val %= 10;
      }
      buf[idx++] = '0' + val;
      buf[idx] = '\0';
      uart_->send_string(buf);
      if (i < 3)
        uart_->send_string(".");
    }
    msg(":");
    char pbuf[6];
    uint16_t p = config_.broker_port;
    uint8_t pidx = 0;
    if (p == 0) {
      pbuf[pidx++] = '0';
    } else {
      char rev[5];
      uint8_t ridx = 0;
      while (p > 0 && ridx < sizeof(rev)) {
        rev[ridx++] = static_cast<char>('0' + (p % 10));
        p /= 10;
      }
      while (ridx > 0) {
        pbuf[pidx++] = rev[--ridx];
      }
    }
    pbuf[pidx] = '\0';
    uart_->send_string(pbuf);
    msg("\r\nClient ID: ");
    uart_->send_string(config_.client_id);
    msg("\r\n");
    return true;
  }

  // "ip 192.168.1.100"
  if (strncmp(cmd, "ip ", 3) == 0) {
    if (parse_ip(cmd + 3, config_.device_ip)) {
      msg("Device IP set\r\n");
      return true;
    }
    msg("Error: Invalid IP\r\n");
    return true;
  }

  // "gw 192.168.1.1"
  if (strncmp(cmd, "gw ", 3) == 0) {
    if (parse_ip(cmd + 3, config_.gateway)) {
      msg("Gateway set\r\n");
      return true;
    }
    msg("Error: Invalid gateway\r\n");
    return true;
  }

  // "br 192.168.1.100:1883" or "br 192.168.1.100 1883"
  if (strncmp(cmd, "br ", 3) == 0) {
    const char* arg = cmd + 3;
    while (*arg == ' ') {
      arg++;
    }

    const char* colon = strchr(arg, ':');
    const char* sep = colon ? colon : strchr(arg, ' ');
    if (sep && parse_ip(arg, config_.broker_ip)) {
      uint16_t port = 0;
      if (parse_port_u16(sep + 1, &port)) {
        config_.broker_port = port;
        msg("Broker set\r\n");
        return true;
      }
    }
    msg("Error: Usage: br <ip>:<port>\r\n");
    return true;
  }

  // "id smartbell123"
  if (strncmp(cmd, "id ", 3) == 0) {
    const char* id = cmd + 3;
    if (*id) {
      strncpy(config_.client_id, id, 15);
      config_.client_id[15] = '\0';
      msg("Client ID set\r\n");
      return true;
    }
    msg("Error: Invalid ID\r\n");
    return true;
  }

  // "save"
  if (strncmp(cmd, "save", 4) == 0) {
    save();
    return true;
  }

  // "reset"
  if (strncmp(cmd, "reset", 5) == 0) {
    reset_to_defaults();
    msg("[CFG] Defaults loaded\r\n");
    return true;
  }

  // Unknown command
  msg("Unknown command\r\n");
  return false;
}

}  // namespace Config
