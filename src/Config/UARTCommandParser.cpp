#include "Config/UARTCommandParser.h"
#include <string.h>
#include "Utils/ProgmemStrings.h"

#ifdef __AVR__
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#else
#define PSTR(s) (s)
#define pgm_read_byte(addr) (*(addr))
#endif

namespace Config {

// PROGMEM strings for size optimization
#ifdef __AVR__
static const char STR_CRLF[] PROGMEM = "\r\n";
static const char STR_PROMPT[] PROGMEM = "> ";
static const char STR_OK[] PROGMEM = "OK\r\n";
static const char STR_ERR[] PROGMEM = "ERR\r\n";
static const char STR_HELP[] PROGMEM =
    "set ip|subnet|gw|broker|port|user|pass|id <val>\r\nshow|save|reboot|help\r\n";
#endif

UARTCommandParser::UARTCommandParser(serial::Interface* uart, ConfigManager* config)
    : uart_(uart),
      config_(config),
      buffer_index_(0),
      reboot_requested_(false),
      config_saved_(false) {
  memset(command_buffer_, 0, kMaxCommandLength);
}

void UARTCommandParser::init(bool show_config_required) {
#ifdef __AVR__
  Utils::println_P(uart_, PSTR("SmartBell Config"));
  if (show_config_required) {
    Utils::println_P(uart_, PSTR("EEPROM empty - type 'help'"));
  }
  Utils::print_P(uart_, STR_PROMPT);
#else
  print_line("SmartBell Config");
  if (show_config_required) {
    print_line("EEPROM empty - type 'help'");
  }
  print("> ");
#endif
}

bool UARTCommandParser::process() {
  if (uart_ == nullptr || !uart_->is_read_data_available()) {
    return false;
  }

  uint8_t c = uart_->read_byte();
  uart_->send(c);

  if (c == 0x08 || c == 0x7F) {
    if (buffer_index_ > 0) {
      buffer_index_--;
      command_buffer_[buffer_index_] = '\0';
      print(" \x08");
    }
    return false;
  }

  if (c == '\r' || c == '\n') {
    print("\r\n");
    if (buffer_index_ > 0) {
      command_buffer_[buffer_index_] = '\0';
      execute_command(command_buffer_);
      memset(command_buffer_, 0, kMaxCommandLength);
      buffer_index_ = 0;
    }
    if (!reboot_requested_) {
      print("> ");
    }
    return true;
  }

  if (buffer_index_ < kMaxCommandLength - 1 && c >= 0x20 && c < 0x7F) {
    command_buffer_[buffer_index_++] = static_cast<char>(c);
  }
  return false;
}

bool UARTCommandParser::reboot_requested() const { return reboot_requested_; }
bool UARTCommandParser::config_saved() const { return config_saved_; }
void UARTCommandParser::clear_flags() {
  reboot_requested_ = false;
  config_saved_ = false;
}

void UARTCommandParser::execute_command(const char* cmd) {
  ParsedCommand parsed = parse_command(cmd);
  uint8_t ip[4];

  switch (parsed.type) {
    case CommandType::kSetIP:
      if (parse_ip(parsed.argument, ip)) {
        config_->set_device_ip(ip);
        print("ip=");
        print_ip(ip);
        print("\r\n");
      } else {
        print("ERR: format a.b.c.d\r\n");
      }
      break;
    case CommandType::kSetSubnet:
      if (parse_ip(parsed.argument, ip)) {
        config_->set_subnet(ip);
        print("subnet=");
        print_ip(ip);
        print("\r\n");
      } else {
        print("ERR: format a.b.c.d\r\n");
      }
      break;
    case CommandType::kSetGateway:
      if (parse_ip(parsed.argument, ip)) {
        config_->set_gateway(ip);
        print("gw=");
        print_ip(ip);
        print("\r\n");
      } else {
        print("ERR: format a.b.c.d\r\n");
      }
      break;
    case CommandType::kSetBrokerIP:
      if (parse_ip(parsed.argument, ip)) {
        config_->set_broker_ip(ip);
        print("broker=");
        print_ip(ip);
        print("\r\n");
      } else {
        print("ERR: format a.b.c.d\r\n");
      }
      break;
    case CommandType::kSetPort:
      config_->set_port(parse_number(parsed.argument));
      print("port=");
      print_number(config_->get_port());
      print("\r\n");
      break;
    case CommandType::kSetUser:
      config_->set_username(parsed.argument);
      print("user=");
      print(parsed.argument);
      print("\r\n");
      break;
    case CommandType::kSetPass:
      config_->set_password(parsed.argument);
      print("pass=***\r\n");
      break;
    case CommandType::kSetClientId:
      config_->set_client_id(parsed.argument);
      print("id=");
      print(parsed.argument);
      print("\r\n");
      break;
    case CommandType::kShowConfig:
      show_config();
      break;
    case CommandType::kSave:
      if (config_->save()) {
        config_saved_ = true;
        print("Saved\r\n");
      } else {
        print("Save failed\r\n");
      }
      break;
    case CommandType::kReboot:
      print("Reboot...\r\n");
      reboot_requested_ = true;
#ifdef __AVR__
      wdt_enable(WDTO_15MS);
      while (1) {
      }
#endif
      break;
    case CommandType::kHelp:
      show_help();
      break;
    case CommandType::kUnknown:
      print("?\r\n");
      break;
    default:
      break;
  }
}

ParsedCommand UARTCommandParser::parse_command(const char* cmd) {
  ParsedCommand result;
  result.type = CommandType::kNone;
  result.argument[0] = '\0';

  cmd = skip_whitespace(cmd);
  if (*cmd == '\0')
    return result;

  const char* arg;

  if ((arg = starts_with(cmd, "set ip ")) != nullptr) {
    result.type = CommandType::kSetIP;
    strncpy(result.argument, skip_whitespace(arg), kMaxCommandLength - 1);
  } else if ((arg = starts_with(cmd, "set subnet ")) != nullptr) {
    result.type = CommandType::kSetSubnet;
    strncpy(result.argument, skip_whitespace(arg), kMaxCommandLength - 1);
  } else if ((arg = starts_with(cmd, "set gw ")) != nullptr) {
    result.type = CommandType::kSetGateway;
    strncpy(result.argument, skip_whitespace(arg), kMaxCommandLength - 1);
  } else if ((arg = starts_with(cmd, "set gateway ")) != nullptr) {
    result.type = CommandType::kSetGateway;
    strncpy(result.argument, skip_whitespace(arg), kMaxCommandLength - 1);
  } else if ((arg = starts_with(cmd, "set broker ")) != nullptr) {
    result.type = CommandType::kSetBrokerIP;
    strncpy(result.argument, skip_whitespace(arg), kMaxCommandLength - 1);
  } else if ((arg = starts_with(cmd, "set port ")) != nullptr) {
    result.type = CommandType::kSetPort;
    strncpy(result.argument, skip_whitespace(arg), kMaxCommandLength - 1);
  } else if ((arg = starts_with(cmd, "set user ")) != nullptr) {
    result.type = CommandType::kSetUser;
    strncpy(result.argument, skip_whitespace(arg), kMaxCommandLength - 1);
  } else if (starts_with(cmd, "set user") != nullptr) {
    result.type = CommandType::kSetUser;
  } else if ((arg = starts_with(cmd, "set pass ")) != nullptr) {
    result.type = CommandType::kSetPass;
    strncpy(result.argument, skip_whitespace(arg), kMaxCommandLength - 1);
  } else if (starts_with(cmd, "set pass") != nullptr) {
    result.type = CommandType::kSetPass;
  } else if ((arg = starts_with(cmd, "set id ")) != nullptr) {
    result.type = CommandType::kSetClientId;
    strncpy(result.argument, skip_whitespace(arg), kMaxCommandLength - 1);
  } else if (starts_with(cmd, "show") != nullptr) {
    result.type = CommandType::kShowConfig;
  } else if (starts_with(cmd, "save") != nullptr) {
    result.type = CommandType::kSave;
  } else if (starts_with(cmd, "reboot") != nullptr) {
    result.type = CommandType::kReboot;
  } else if (starts_with(cmd, "help") != nullptr) {
    result.type = CommandType::kHelp;
  } else {
    result.type = CommandType::kUnknown;
  }
  return result;
}

void UARTCommandParser::show_config() {
  // Compact output to save flash
  print_ip(config_->get_device_ip());
  print(" ");
  print_ip(config_->get_broker_ip());
  print(":");
  print_number(config_->get_port());
  print("\r\n");
  print(config_->is_valid() ? "OK\r\n" : "ERR\r\n");
}

void UARTCommandParser::show_help() {
#ifdef __AVR__
  Utils::print_P(uart_, STR_HELP);
#else
  print("set ip|gw|broker|port|user|pass|id <val>\r\n");
#endif
}

void UARTCommandParser::print(const char* str) {
  if (uart_ != nullptr)
    uart_->send_string(str);
}

void UARTCommandParser::print_line(const char* str) {
  print(str);
  print("\r\n");
}

void UARTCommandParser::print_number(uint16_t value) {
  char buf[6];
  int i = 5;
  buf[i] = '\0';
  if (value == 0) {
    print("0");
    return;
  }
  while (value > 0 && i > 0) {
    buf[--i] = '0' + (value % 10);
    value /= 10;
  }
  print(&buf[i]);
}

char UARTCommandParser::to_lower(char c) { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }

uint16_t UARTCommandParser::parse_number(const char* str) {
  uint16_t result = 0;
  str = skip_whitespace(str);
  while (*str >= '0' && *str <= '9') {
    result = result * 10 + (*str - '0');
    str++;
  }
  return result;
}

const char* UARTCommandParser::skip_whitespace(const char* str) {
  while (*str == ' ' || *str == '\t')
    str++;
  return str;
}

const char* UARTCommandParser::starts_with(const char* str, const char* prefix) {
  while (*prefix) {
    if (to_lower(*str) != to_lower(*prefix))
      return nullptr;
    str++;
    prefix++;
  }
  return str;
}

bool UARTCommandParser::parse_ip(const char* str, uint8_t* ip_out) {
  uint8_t octet = 0;
  uint8_t octet_index = 0;
  uint8_t digit_count = 0;

  str = skip_whitespace(str);

  for (size_t i = 0; str[i] != '\0' && octet_index < 4; i++) {
    if (str[i] >= '0' && str[i] <= '9') {
      octet = octet * 10 + (str[i] - '0');
      digit_count++;
      if (digit_count > 3 || octet > 255)
        return false;
    } else if (str[i] == '.') {
      if (digit_count == 0)
        return false;
      ip_out[octet_index++] = octet;
      octet = 0;
      digit_count = 0;
    } else {
      return false;
    }
  }

  if (digit_count > 0 && octet_index == 3) {
    ip_out[octet_index] = octet;
    return true;
  }
  return false;
}

void UARTCommandParser::print_ip(const uint8_t* ip) {
  for (int i = 0; i < 4; i++) {
    print_number(ip[i]);
    if (i < 3)
      print(".");
  }
}

}  // namespace Config
