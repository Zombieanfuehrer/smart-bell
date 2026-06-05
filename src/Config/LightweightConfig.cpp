// Lightweight ConfigManager - Optimized for minimal SRAM usage
// Uses manual string parsing to avoid vfprintf_std/vfscanf_std (~800B)

#include "Config/LightweightConfig.h"

#include <stddef.h>  // for size_t
#include <string.h>

// Platform-specific includes
#ifdef __AVR__
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>
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
#include <avr/pgmspace.h>

extern "C" {
extern const char smart_bell_flash_help[] __attribute__((__progmem__));
}

void print_help_from_flash(serial::Interface* uart_ptr) {
  if (!uart_ptr)
    return;
  const char* p = smart_bell_flash_help;
  char ch;
  // Liest den String direkt mittels pgm_read_byte über die Flash-Adresse aus
  while ((ch = pgm_read_byte(p++)) != '\0') {
    uart_ptr->send(static_cast<uint8_t>(ch));
  }
}

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

LightweightConfig::LightweightConfig(serial::Interface& uart) : uart_(&uart) {
  reset_to_defaults();
}

void LightweightConfig::msg_ptr(const char* progmem_text) {
  if (!uart_)
    return;
  char ch;
  while ((ch = pgm_read_byte(progmem_text++)) != '\0') {
    uart_->send(static_cast<uint8_t>(ch));
  }
}

void LightweightConfig::reset_to_defaults() {
  // MQTT defaults - all zero = not configured (user must set with 'br' command)
  config_.broker_ip[0] = 0;
  config_.broker_ip[1] = 0;
  config_.broker_ip[2] = 0;
  config_.broker_ip[3] = 0;
  config_.broker_port = 1883;
  strncpy_P(config_.client_id, PSTR("smartbell"), 15);
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
    msg_ptr(PSTR("[CFG] Loaded\r\n"));
    return;
  }

  // One-time migration path for older firmware using EEPROM address 0.
  eeprom_read_cfg(&temp, EEPROM_ADDR_LEGACY);
  if (is_valid_config(temp)) {
    memcpy(&config_, &temp, sizeof(SmartBellConfig));
    eeprom_write_cfg(&config_, EEPROM_ADDR);
    msg_ptr(PSTR("[CFG] Loaded (migrated)\r\n"));
    return;
  }

  msg_ptr(PSTR("[CFG] Invalid, using defaults\r\n"));
}

void LightweightConfig::save() {
  config_.magic = MAGIC;
  eeprom_write_cfg(&config_, EEPROM_ADDR);
  save_flag_ = true;
  msg_ptr(PSTR("[CFG] Saved\r\n"));
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

// Hilfsfunktion zum IP-Drucken (spart extrem viel Flash-Speicher bei 3-facher Nutzung)
void LightweightConfig::print_ip(const uint8_t* ip) {
  char buf[4];
  for (int i = 0; i < 4; i++) {
    uint8_t val = ip[i];
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
}

// Simple command parser - erweiterte Version
bool LightweightConfig::process_command(const char* cmd) {
  if (!cmd || !uart_)
    return false;

  // Skip leading whitespace
  while (*cmd == ' ')
    cmd++;

  // "help"
  if (strncmp_P(cmd, PSTR("help"), 4) == 0) {
#ifdef __AVR__
    print_help_from_flash(uart_);
#else
    uart_->send_string(smart_bell_flash_help);
#endif
    return true;
  }

  // "show" (optimiert mit der print_ip Funktion)
  if (strncmp_P(cmd, PSTR("show"), 4) == 0) {
    msg_ptr(PSTR("Device IP: "));
    print_ip(config_.device_ip);

    msg_ptr(PSTR("\r\nGateway: "));
    print_ip(config_.gateway);

    msg_ptr(PSTR("\r\nBroker: "));
    print_ip(config_.broker_ip);
    msg_ptr(PSTR(":"));

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

    msg_ptr(PSTR("\r\nClient ID: "));
    uart_->send_string(config_.client_id);

    msg_ptr(PSTR("\r\nInput 1 Topic: "));
    uart_->send_string(config_.input1_topic);

    msg_ptr(PSTR("\r\nInput 2 Topic: "));
    uart_->send_string(config_.input2_topic);

    msg_ptr(PSTR("\r\nGong Base Topic: "));
    uart_->send_string(config_.gong_base_topic);

    msg_ptr(PSTR("\r\n"));
    return true;
  }

  // "ip 192.168.1.100"
  if (strncmp_P(cmd, PSTR("ip "), 3) == 0) {
    if (parse_ip(cmd + 3, config_.device_ip)) {
      msg_ptr(PSTR("Device IP set\r\n"));
      return true;
    }
    msg_ptr(PSTR("Error: Invalid IP\r\n"));
    return true;
  }

  // "sn 255.255.255.0"
  if (strncmp_P(cmd, PSTR("sn "), 3) == 0) {
    if (parse_ip(cmd + 3, config_.subnet)) {
      msg_ptr(PSTR("Subnet mask set\r\n"));
      return true;
    }
    msg_ptr(PSTR("Error: Invalid subnet mask\r\n"));
    return true;
  }

  // "gw 192.168.1.1"
  if (strncmp_P(cmd, PSTR("gw "), 3) == 0) {
    if (parse_ip(cmd + 3, config_.gateway)) {
      msg_ptr(PSTR("Gateway set\r\n"));
      return true;
    }
    msg_ptr(PSTR("Error: Invalid gateway\r\n"));
    return true;
  }

  // "br 192.168.1.100:1883"
  if (strncmp_P(cmd, PSTR("br "), 3) == 0) {
    const char* arg = cmd + 3;
    while (*arg == ' ')
      arg++;

    const char* colon = strchr(arg, ':');
    const char* sep = colon ? colon : strchr(arg, ' ');
    if (sep && parse_ip(arg, config_.broker_ip)) {
      uint16_t port = 0;
      if (parse_port_u16(sep + 1, &port)) {
        config_.broker_port = port;
        msg_ptr(PSTR("Broker set\r\n"));
        return true;
      }
    }
    msg_ptr(PSTR("Error: Usage: br <ip>:<port>\r\n"));
    return true;
  }

  // "id <name>"
  if (strncmp_P(cmd, PSTR("id "), 3) == 0) {
    const char* id = cmd + 3;
    while (*id == ' ')
      id++;
    if (*id) {
      strncpy(config_.client_id, id, 15);
      config_.client_id[15] = '\0';
      msg_ptr(PSTR("Client ID set\r\n"));
      return true;
    }
    msg_ptr(PSTR("Error: Invalid ID\r\n"));
    return true;
  }

  // "input 1 pt <topic>" und "input 2 pt <topic>"
  if (strncmp_P(cmd, PSTR("input "), 6) == 0) {
    const char* arg = cmd + 6;
    bool is_input1 = (strncmp_P(arg, PSTR("1 pt "), 5) == 0);
    bool is_input2 = (strncmp_P(arg, PSTR("2 pt "), 5) == 0);

    if (is_input1 || is_input2) {
      const char* t = arg + 5;
      while (*t == ' ')
        t++;

      if (*t && *t != '\r' && *t != '\n') {
        // Ziel-Array und maximale Länge bestimmen (24 Zeichen)
        char* dest = is_input1 ? config_.input1_topic : config_.input2_topic;
        constexpr size_t max_len = 24;

        // Kopieren und händisch nach max_len Zeichen oder bei Zeilenumbruch stoppen
        size_t len = 0;
        while (t[len] != '\0' && t[len] != '\r' && t[len] != '\n' && len < max_len) {
          dest[len] = t[len];
          len++;
        }
        dest[len] = '\0';  // Sichere Nullterminierung

        if (is_input1)
          msg_ptr(PSTR("Input 1 Topic set\r\n"));
        else
          msg_ptr(PSTR("Input 2 Topic set\r\n"));
        return true;
      }
    }
    msg_ptr(PSTR("Error: Usage: input [1|2] pt <topic>\r\n"));
    return true;
  }

  // "gong sub <topic>"
  if (strncmp_P(cmd, PSTR("gong sub "), 9) == 0) {
    const char* t = cmd + 9;
    while (*t == ' ')
      t++;

    if (*t && *t != '\r' && *t != '\n') {
      constexpr size_t max_len = 24;
      size_t len = 0;

      while (t[len] != '\0' && t[len] != '\r' && t[len] != '\n' && len < max_len) {
        config_.gong_base_topic[len] = t[len];
        len++;
      }
      config_.gong_base_topic[len] = '\0';  // Sichere Nullterminierung

      msg_ptr(PSTR("Gong Base Topic set\r\n"));
      return true;
    }
    msg_ptr(PSTR("Error: Invalid topic\r\n"));
    return true;
  }

  // "save"
  if (strncmp_P(cmd, PSTR("save"), 4) == 0) {
    save();
    msg_ptr(PSTR("Config saved to EEPROM\r\n"));
    return true;
  }

  // "reset"
  if (strncmp_P(cmd, PSTR("reset"), 5) == 0) {
    reset_to_defaults();
    msg_ptr(PSTR("[CFG] Defaults loaded\r\n"));
    return true;
  }

  // "reboot"
  if (strncmp_P(cmd, PSTR("reboot"), 6) == 0) {
    msg_ptr(PSTR("Rebooting...\r\n"));
#ifdef __AVR__
    _delay_ms(100);
    wdt_enable(WDTO_15MS);
    while (1)
      ;
#endif
    return true;
  }

  // Unknown command
  msg_ptr(PSTR("Unknown command\r\n"));
  return false;
}

}  // namespace Config
