#include "Config/UARTCommandParser.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "Config/ConfigManager.h"

using namespace Config;

/**
 * @brief Mock UART interface for testing.
 */
class MockUART : public serial::Interface {
 public:
  MockUART() : read_index_(0) {}

  void send(const uint8_t byte) override { output_.push_back(static_cast<char>(byte)); }

  void send_bytes(const uint8_t* const bytes, const uint16_t length) override {
    for (uint16_t i = 0; i < length; i++) {
      output_.push_back(static_cast<char>(bytes[i]));
    }
  }

  void send_string(const char* string) override {
    while (*string) {
      output_.push_back(*string++);
    }
  }

  bool is_read_data_available() const override { return read_index_ < input_.size(); }

  uint8_t read_byte() override {
    if (read_index_ < input_.size()) {
      return static_cast<uint8_t>(input_[read_index_++]);
    }
    return 0;
  }

  // Test helper methods
  void set_input(const std::string& input) {
    input_ = input;
    read_index_ = 0;
  }

  std::string get_output() const { return output_; }

  void clear_output() { output_.clear(); }

  void clear_all() {
    input_.clear();
    output_.clear();
    read_index_ = 0;
  }

 private:
  std::string input_;
  std::string output_;
  size_t read_index_;
};

class UARTCommandParserTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mock_uart_ = new MockUART();
    config_manager_ = new ConfigManager();
    parser_ = new UARTCommandParser(mock_uart_, config_manager_);
  }

  void TearDown() override {
    delete parser_;
    delete config_manager_;
    delete mock_uart_;
  }

  void simulate_command(const std::string& command) {
    mock_uart_->clear_all();
    mock_uart_->set_input(command + "\r");

    // Process until command is complete
    while (mock_uart_->is_read_data_available()) {
      parser_->process();
    }
  }

  MockUART* mock_uart_;
  ConfigManager* config_manager_;
  UARTCommandParser* parser_;
};

// Test initialization
TEST_F(UARTCommandParserTest, InitDisplaysWelcome) {
  parser_->init(false);

  std::string output = mock_uart_->get_output();
  EXPECT_NE(output.find("SmartBell"), std::string::npos);
  EXPECT_NE(output.find("Configuration"), std::string::npos);
}

// Test set broker command with IP
TEST_F(UARTCommandParserTest, SetBrokerIP) {
  parser_->init(false);
  mock_uart_->clear_output();

  simulate_command("set broker 192.168.1.100");

  EXPECT_STREQ(config_manager_->get_broker(), "192.168.1.100");
  EXPECT_FALSE(config_manager_->broker_is_hostname());

  std::string output = mock_uart_->get_output();
  EXPECT_NE(output.find("Broker set to"), std::string::npos);
  EXPECT_NE(output.find("IP address"), std::string::npos);
}

// Test set broker command with hostname
TEST_F(UARTCommandParserTest, SetBrokerHostname) {
  parser_->init(false);
  mock_uart_->clear_output();

  simulate_command("set broker mqtt.example.com");

  EXPECT_STREQ(config_manager_->get_broker(), "mqtt.example.com");
  EXPECT_TRUE(config_manager_->broker_is_hostname());

  std::string output = mock_uart_->get_output();
  EXPECT_NE(output.find("hostname"), std::string::npos);
}

// Test set port command
TEST_F(UARTCommandParserTest, SetPort) {
  parser_->init(false);
  mock_uart_->clear_output();

  simulate_command("set port 8883");

  EXPECT_EQ(config_manager_->get_port(), 8883);

  std::string output = mock_uart_->get_output();
  EXPECT_NE(output.find("Port set to"), std::string::npos);
}

// Test set user command
TEST_F(UARTCommandParserTest, SetUser) {
  parser_->init(false);
  mock_uart_->clear_output();

  simulate_command("set user myuser");

  EXPECT_STREQ(config_manager_->get_username(), "myuser");

  std::string output = mock_uart_->get_output();
  EXPECT_NE(output.find("Username set"), std::string::npos);
}

// Test set pass command
TEST_F(UARTCommandParserTest, SetPass) {
  parser_->init(false);
  mock_uart_->clear_output();

  simulate_command("set pass secret123");

  EXPECT_STREQ(config_manager_->get_password(), "secret123");

  std::string output = mock_uart_->get_output();
  EXPECT_NE(output.find("Password set"), std::string::npos);
}

// Test set clientid command
TEST_F(UARTCommandParserTest, SetClientId) {
  parser_->init(false);
  mock_uart_->clear_output();

  simulate_command("set clientid bell-01");

  EXPECT_STREQ(config_manager_->get_client_id(), "bell-01");

  std::string output = mock_uart_->get_output();
  EXPECT_NE(output.find("Client ID set"), std::string::npos);
}

// Test show config command
TEST_F(UARTCommandParserTest, ShowConfig) {
  config_manager_->set_broker("192.168.1.50");
  config_manager_->set_port(1883);
  config_manager_->set_username("testuser");
  config_manager_->set_password("testpass");

  parser_->init(false);
  mock_uart_->clear_output();

  simulate_command("show config");

  std::string output = mock_uart_->get_output();
  EXPECT_NE(output.find("192.168.1.50"), std::string::npos);
  EXPECT_NE(output.find("1883"), std::string::npos);
  EXPECT_NE(output.find("testuser"), std::string::npos);
  EXPECT_NE(output.find("********"), std::string::npos);  // Password hidden
}

// Test help command
TEST_F(UARTCommandParserTest, HelpCommand) {
  parser_->init(false);
  mock_uart_->clear_output();

  simulate_command("help");

  std::string output = mock_uart_->get_output();
  EXPECT_NE(output.find("set broker"), std::string::npos);
  EXPECT_NE(output.find("set port"), std::string::npos);
  EXPECT_NE(output.find("save"), std::string::npos);
  EXPECT_NE(output.find("reboot"), std::string::npos);
}

// Test save command
TEST_F(UARTCommandParserTest, SaveCommand) {
  config_manager_->set_broker("192.168.1.1");

  parser_->init(false);
  mock_uart_->clear_output();

  EXPECT_FALSE(parser_->config_saved());

  simulate_command("save");

  EXPECT_TRUE(parser_->config_saved());

  std::string output = mock_uart_->get_output();
  EXPECT_NE(output.find("saved"), std::string::npos);
}

// Test unknown command
TEST_F(UARTCommandParserTest, UnknownCommand) {
  parser_->init(false);
  mock_uart_->clear_output();

  simulate_command("invalid command");

  std::string output = mock_uart_->get_output();
  EXPECT_NE(output.find("Unknown command"), std::string::npos);
}

// Test case insensitivity
TEST_F(UARTCommandParserTest, CaseInsensitive) {
  parser_->init(false);
  mock_uart_->clear_output();

  simulate_command("SET BROKER 10.0.0.1");

  EXPECT_STREQ(config_manager_->get_broker(), "10.0.0.1");
}

// Test empty command
TEST_F(UARTCommandParserTest, EmptyCommand) {
  parser_->init(false);
  mock_uart_->clear_output();

  simulate_command("");

  // Should just show prompt again, no error
  std::string output = mock_uart_->get_output();
  EXPECT_EQ(output.find("Unknown"), std::string::npos);
}

// Test clear flags
TEST_F(UARTCommandParserTest, ClearFlags) {
  config_manager_->set_broker("test.com");
  parser_->init(false);

  simulate_command("save");
  EXPECT_TRUE(parser_->config_saved());

  parser_->clear_flags();
  EXPECT_FALSE(parser_->config_saved());
}
