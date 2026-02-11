#ifndef PUBLIC_CONFIG_UARTCOMMANDPARSER_H_
#define PUBLIC_CONFIG_UARTCOMMANDPARSER_H_

#include <stdint.h>
#include "Config/ConfigManager.h"
#include "Serial/Interface.h"

namespace Config {

// Command buffer size
static constexpr uint8_t kMaxCommandLength = 80;

/**
 * @brief Command types recognized by the parser.
 */
enum class CommandType : uint8_t {
  kNone = 0,
  kSetIP,
  kSetSubnet,
  kSetGateway,
  kSetBrokerIP,
  kSetPort,
  kSetUser,
  kSetPass,
  kSetClientId,
  kShowConfig,
  kSave,
  kReboot,
  kHelp,
  kUnknown
};

/**
 * @brief Result of command parsing.
 */
struct ParsedCommand {
  CommandType type;
  char argument[kMaxCommandLength];
};

/**
 * @brief UART-based command line interface for configuration.
 *
 * Provides an interactive menu for configuring MQTT broker settings
 * via UART. Commands are line-based (terminated by CR or LF).
 *
 * Supported commands:
 * - set broker <ip/hostname>  : Set MQTT broker address
 * - set port <port>           : Set MQTT broker port
 * - set user <username>       : Set MQTT username
 * - set pass <password>       : Set MQTT password
 * - set clientid <id>         : Set MQTT client ID
 * - show config               : Display current configuration
 * - save                      : Save configuration to EEPROM
 * - reboot                    : Restart the device
 * - help                      : Show available commands
 */
class UARTCommandParser {
 public:
  /**
   * @brief Construct parser with UART interface and config manager.
   * @param uart UART interface for input/output.
   * @param config Configuration manager.
   */
  UARTCommandParser(serial::Interface* uart, ConfigManager* config);
  ~UARTCommandParser() = default;

  /**
   * @brief Initialize and display welcome message.
   * @param show_config_required If true, indicates EEPROM is empty.
   */
  void init(bool show_config_required);

  /**
   * @brief Process pending UART input.
   * Call this in the main loop to handle incoming characters.
   * @return true if a complete command was processed.
   */
  bool process();

  /**
   * @brief Check if reboot was requested.
   * @return true if reboot command was executed.
   */
  bool reboot_requested() const;

  /**
   * @brief Check if configuration was saved.
   * @return true if save command was executed successfully.
   */
  bool config_saved() const;

  /**
   * @brief Reset the saved/reboot flags.
   */
  void clear_flags();

  /**
   * @brief Display current configuration.
   */
  void show_config();

  /**
   * @brief Display help message.
   */
  void show_help();

 private:
  serial::Interface* uart_;
  ConfigManager* config_;

  char command_buffer_[kMaxCommandLength];
  uint8_t buffer_index_;
  bool reboot_requested_;
  bool config_saved_;

  /**
   * @brief Parse and execute a complete command line.
   * @param command_line Null-terminated command string.
   */
  void execute_command(const char* command_line);

  /**
   * @brief Parse command type and argument from command line.
   * @param command_line Input command string.
   * @return Parsed command structure.
   */
  ParsedCommand parse_command(const char* command_line);

  /**
   * @brief Check if string starts with a prefix (case-insensitive).
   * @param str String to check.
   * @param prefix Prefix to match.
   * @return Pointer past the prefix if matched, nullptr otherwise.
   */
  const char* starts_with(const char* str, const char* prefix);

  /**
   * @brief Skip leading whitespace.
   * @param str String to process.
   * @return Pointer to first non-whitespace character.
   */
  const char* skip_whitespace(const char* str);

  /**
   * @brief Send a string to UART.
   * @param str Null-terminated string.
   */
  void print(const char* str);

  /**
   * @brief Send a formatted line with prefix.
   * @param message Message to print.
   */
  void print_line(const char* message);

  /**
   * @brief Print a number in decimal.
   * @param value Number to print.
   */
  void print_number(uint16_t value);

  /**
   * @brief Convert character to lowercase.
   * @param c Character.
   * @return Lowercase character.
   */
  char to_lower(char c);

  /**
   * @brief Parse decimal number from string.
   * @param str String containing number.
   * @return Parsed number, 0 on error.
   */
  uint16_t parse_number(const char* str);

  /**
   * @brief Parse IP address from string.
   * @param str IP string like "192.168.1.100".
   * @param ip_out Output buffer for 4 bytes.
   * @return true if parsed successfully.
   */
  bool parse_ip(const char* str, uint8_t* ip_out);

  /**
   * @brief Print IP address.
   * @param ip 4-byte IP address.
   */
  void print_ip(const uint8_t* ip);
};

}  // namespace Config

#endif  // PUBLIC_CONFIG_UARTCOMMANDPARSER_H_
