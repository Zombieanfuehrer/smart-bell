#include "Config/UARTCommandParser.h"
#include <string.h>

#ifdef __AVR__
#include <avr/wdt.h>
#endif

namespace Config {

UARTCommandParser::UARTCommandParser(serial::Interface* uart, ConfigManager* config)
    : uart_(uart),
      config_(config),
      buffer_index_(0),
      reboot_requested_(false),
      config_saved_(false) {
  memset(command_buffer_, 0, kMaxCommandLength);
}

void UARTCommandParser::init(bool show_config_required) {
  print("\r\n");
  print_line("========================================");
  print_line("SmartBell Configuration Menu");
  print_line("========================================");

  if (show_config_required) {
    print_line("EEPROM empty - please configure:");
    print_line("Use 'help' to see available commands");
  } else {
    print_line("Type 'help' for available commands");
  }

  print("[CONFIG] > ");
}

bool UARTCommandParser::process() {
  if (uart_ == nullptr || !uart_->is_read_data_available()) {
    return false;
  }

  uint8_t c = uart_->read_byte();

  // Echo character
  uart_->send(c);

  // Handle backspace
  if (c == 0x08 || c == 0x7F) {  // Backspace or DEL
    if (buffer_index_ > 0) {
      buffer_index_--;
      command_buffer_[buffer_index_] = '\0';
      // Erase character on terminal: backspace, space, backspace
      print(" \x08");
    }
    return false;
  }

  // Handle Enter (CR or LF)
  if (c == '\r' || c == '\n') {
    print("\r\n");

    if (buffer_index_ > 0) {
      command_buffer_[buffer_index_] = '\0';
      execute_command(command_buffer_);
      memset(command_buffer_, 0, kMaxCommandLength);
      buffer_index_ = 0;
    }

    if (!reboot_requested_) {
      print("[CONFIG] > ");
    }
    return true;
  }

  // Add character to buffer
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

void UARTCommandParser::execute_command(const char* command_line) {
  ParsedCommand cmd = parse_command(command_line);

  switch (cmd.type) {
    case CommandType::kSetBroker:
      if (cmd.argument[0] != '\0') {
        config_->set_broker(cmd.argument);
        print("[CONFIG] Broker set to: ");
        print(cmd.argument);
        if (config_->broker_is_hostname()) {
          print(" (hostname)");
        } else {
          print(" (IP address)");
        }
        print("\r\n");
      } else {
        print_line("Error: Broker address required");
      }
      break;

    case CommandType::kSetPort: {
      uint16_t port = parse_number(cmd.argument);
      if (port > 0 && port <= 65535) {
        config_->set_port(port);
        print("[CONFIG] Port set to: ");
        print_number(port);
        print("\r\n");
      } else {
        print_line("Error: Invalid port number (1-65535)");
      }
      break;
    }

    case CommandType::kSetUser:
      if (cmd.argument[0] != '\0') {
        config_->set_username(cmd.argument);
        print("[CONFIG] Username set to: ");
        print(cmd.argument);
        print("\r\n");
      } else {
        config_->set_username("");
        print_line("Username cleared (no authentication)");
      }
      break;

    case CommandType::kSetPass:
      if (cmd.argument[0] != '\0') {
        config_->set_password(cmd.argument);
        print_line("Password set");
      } else {
        config_->set_password("");
        print_line("Password cleared");
      }
      break;

    case CommandType::kSetClientId:
      if (cmd.argument[0] != '\0') {
        config_->set_client_id(cmd.argument);
        print("[CONFIG] Client ID set to: ");
        print(cmd.argument);
        print("\r\n");
      } else {
        print_line("Error: Client ID required");
      }
      break;

    case CommandType::kShowConfig:
      show_config();
      break;

    case CommandType::kSave:
      print_line("Saving to EEPROM...");
      if (config_->save()) {
        print_line("Configuration saved successfully");
        config_saved_ = true;
      } else {
        print_line("Error: Failed to save configuration");
      }
      break;

    case CommandType::kReboot:
      print_line("Rebooting...");
      reboot_requested_ = true;
#ifdef __AVR__
      // Force a watchdog reset
      wdt_enable(WDTO_15MS);
      while (1) {
      }
#endif
      break;

    case CommandType::kHelp:
      show_help();
      break;

    case CommandType::kUnknown:
      print("[CONFIG] Unknown command: ");
      print(command_line);
      print("\r\n");
      print_line("Type 'help' for available commands");
      break;

    case CommandType::kNone:
    default:
      break;
  }
}

ParsedCommand UARTCommandParser::parse_command(const char* command_line) {
  ParsedCommand result;
  result.type = CommandType::kNone;
  result.argument[0] = '\0';

  const char* cmd = skip_whitespace(command_line);
  if (*cmd == '\0') {
    return result;
  }

  const char* arg;

  // Check for "set" commands
  if ((arg = starts_with(cmd, "set broker ")) != nullptr) {
    result.type = CommandType::kSetBroker;
    arg = skip_whitespace(arg);
    strncpy(result.argument, arg, kMaxCommandLength - 1);
  } else if ((arg = starts_with(cmd, "set port ")) != nullptr) {
    result.type = CommandType::kSetPort;
    arg = skip_whitespace(arg);
    strncpy(result.argument, arg, kMaxCommandLength - 1);
  } else if ((arg = starts_with(cmd, "set user ")) != nullptr) {
    result.type = CommandType::kSetUser;
    arg = skip_whitespace(arg);
    strncpy(result.argument, arg, kMaxCommandLength - 1);
  } else if ((arg = starts_with(cmd, "set user")) != nullptr && *skip_whitespace(arg) == '\0') {
    result.type = CommandType::kSetUser;  // Clear username
  } else if ((arg = starts_with(cmd, "set pass ")) != nullptr) {
    result.type = CommandType::kSetPass;
    arg = skip_whitespace(arg);
    strncpy(result.argument, arg, kMaxCommandLength - 1);
  } else if ((arg = starts_with(cmd, "set pass")) != nullptr && *skip_whitespace(arg) == '\0') {
    result.type = CommandType::kSetPass;  // Clear password
  } else if ((arg = starts_with(cmd, "set clientid ")) != nullptr) {
    result.type = CommandType::kSetClientId;
    arg = skip_whitespace(arg);
    strncpy(result.argument, arg, kMaxCommandLength - 1);
  }
  // Single-word commands
  else if (starts_with(cmd, "show config") != nullptr || starts_with(cmd, "show") != nullptr) {
    result.type = CommandType::kShowConfig;
  } else if (starts_with(cmd, "save") != nullptr) {
    result.type = CommandType::kSave;
  } else if (starts_with(cmd, "reboot") != nullptr || starts_with(cmd, "restart") != nullptr) {
    result.type = CommandType::kReboot;
  } else if (starts_with(cmd, "help") != nullptr || starts_with(cmd, "?") != nullptr) {
    result.type = CommandType::kHelp;
  } else {
    result.type = CommandType::kUnknown;
  }

  return result;
}

const char* UARTCommandParser::starts_with(const char* str, const char* prefix) {
  while (*prefix != '\0') {
    if (to_lower(*str) != to_lower(*prefix)) {
      return nullptr;
    }
    str++;
    prefix++;
  }
  return str;
}

const char* UARTCommandParser::skip_whitespace(const char* str) {
  while (*str == ' ' || *str == '\t') {
    str++;
  }
  return str;
}

void UARTCommandParser::print(const char* str) {
  if (uart_ != nullptr) {
    uart_->send_string(str);
  }
}

void UARTCommandParser::print_line(const char* message) {
  print("[CONFIG] ");
  print(message);
  print("\r\n");
}

void UARTCommandParser::print_number(uint16_t value) {
  char buffer[6];  // Max 65535 + null
  int i = 5;
  buffer[i] = '\0';

  if (value == 0) {
    print("0");
    return;
  }

  while (value > 0 && i > 0) {
    i--;
    buffer[i] = '0' + (value % 10);
    value /= 10;
  }

  print(&buffer[i]);
}

char UARTCommandParser::to_lower(char c) {
  if (c >= 'A' && c <= 'Z') {
    return c + ('a' - 'A');
  }
  return c;
}

uint16_t UARTCommandParser::parse_number(const char* str) {
  uint16_t result = 0;
  str = skip_whitespace(str);

  while (*str >= '0' && *str <= '9') {
    uint16_t digit = *str - '0';
    // Check for overflow
    if (result > (65535 - digit) / 10) {
      return 0;  // Overflow
    }
    result = result * 10 + digit;
    str++;
  }

  return result;
}

void UARTCommandParser::show_config() {
  print_line("----------------------------------------");
  print_line("Current Configuration:");
  print_line("----------------------------------------");

  print("[CONFIG] Broker:    ");
  if (config_->get_broker()[0] != '\0') {
    print(config_->get_broker());
    if (config_->broker_is_hostname()) {
      print(" (hostname, DNS required)");
    }
  } else {
    print("<not set>");
  }
  print("\r\n");

  print("[CONFIG] Port:      ");
  print_number(config_->get_port());
  print("\r\n");

  print("[CONFIG] Username:  ");
  if (config_->get_username()[0] != '\0') {
    print(config_->get_username());
  } else {
    print("<not set>");
  }
  print("\r\n");

  print("[CONFIG] Password:  ");
  if (config_->get_password()[0] != '\0') {
    print("********");
  } else {
    print("<not set>");
  }
  print("\r\n");

  print("[CONFIG] Client ID: ");
  print(config_->get_client_id());
  print("\r\n");

  print_line("----------------------------------------");

  if (config_->is_valid()) {
    print_line("Status: Configuration valid");
  } else {
    print_line("Status: Configuration incomplete");
  }
}

void UARTCommandParser::show_help() {
  print_line("----------------------------------------");
  print_line("Available Commands:");
  print_line("----------------------------------------");
  print_line("set broker <ip/hostname>  - Set MQTT broker");
  print_line("set port <port>           - Set MQTT port (default: 1883)");
  print_line("set user <username>       - Set MQTT username");
  print_line("set pass <password>       - Set MQTT password");
  print_line("set clientid <id>         - Set MQTT client ID");
  print_line("show                      - Show current configuration");
  print_line("save                      - Save to EEPROM");
  print_line("reboot                    - Restart device");
  print_line("help                      - Show this help");
  print_line("----------------------------------------");
}

}  // namespace Config
