#include <gtest/gtest.h>

// Include hardware mock before any AVR headers
#include "../Mocks/AVRHardwareMock.h"

#include <cstring>
#include <sstream>
#include "Config/LightweightConfig.h"
#include "Serial/Interface.h"

namespace {

// Mock UART for testing
class MockUART : public serial::Interface {
 public:
  void send(const uint8_t byte) override { output_ += static_cast<char>(byte); }

  void send_bytes(const uint8_t *const bytes, const uint16_t length) override {
    for (uint16_t i = 0; i < length; i++) {
      output_ += static_cast<char>(bytes[i]);
    }
  }

  void send_string(const char *string) override { output_ += string; }

  bool is_read_data_available() const override { return input_pos_ < input_.length(); }

  uint8_t read_byte() override {
    if (input_pos_ < input_.length()) {
      return static_cast<uint8_t>(input_[input_pos_++]);
    }
    return 0;
  }

  // Test helper methods
  std::string get_output() const { return output_; }
  void clear_output() { output_.clear(); }
  void set_input(const std::string &input) {
    input_ = input;
    input_pos_ = 0;
  }

 private:
  std::string output_;
  std::string input_;
  size_t input_pos_ = 0;
};

class LightweightConfigTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mock_uart_ = std::make_unique<MockUART>();
    config_ = std::make_unique<Config::LightweightConfig>(*mock_uart_);

    // Clear EEPROM mock before each test
    mock::EEPROMMock::instance().clear();
  }

  void TearDown() override {
    config_.reset();
    mock_uart_.reset();
  }

  std::unique_ptr<MockUART> mock_uart_;
  std::unique_ptr<Config::LightweightConfig> config_;
};

TEST_F(LightweightConfigTest, DefaultConfiguration) {
  auto cfg = config_->config();

  // Default IP: 192.168.1.100
  EXPECT_EQ(cfg.device_ip[0], 192);
  EXPECT_EQ(cfg.device_ip[1], 168);
  EXPECT_EQ(cfg.device_ip[2], 1);
  EXPECT_EQ(cfg.device_ip[3], 100);

  // Default Gateway: 192.168.1.1
  EXPECT_EQ(cfg.gateway[0], 192);
  EXPECT_EQ(cfg.gateway[1], 168);
  EXPECT_EQ(cfg.gateway[2], 1);
  EXPECT_EQ(cfg.gateway[3], 1);

  // Default Broker: 192.168.1.10:1883
  EXPECT_EQ(cfg.broker_ip[0], 192);
  EXPECT_EQ(cfg.broker_ip[1], 168);
  EXPECT_EQ(cfg.broker_ip[2], 1);
  EXPECT_EQ(cfg.broker_ip[3], 10);
  EXPECT_EQ(cfg.broker_port, 1883);

  // Default Client ID
  EXPECT_STREQ(cfg.client_id, "smartbell");
}

TEST_F(LightweightConfigTest, HelpCommand) {
  mock_uart_->set_input("help\n");
  config_->process_command("help");

  std::string output = mock_uart_->get_output();
  EXPECT_TRUE(output.find("Commands:") != std::string::npos);
  EXPECT_TRUE(output.find("show") != std::string::npos);
  EXPECT_TRUE(output.find("save") != std::string::npos);
}

TEST_F(LightweightConfigTest, ShowCommand) {
  config_->process_command("show");

  std::string output = mock_uart_->get_output();
  EXPECT_TRUE(output.find("Device IP:") != std::string::npos);
  EXPECT_TRUE(output.find("Gateway:") != std::string::npos);
  EXPECT_TRUE(output.find("Broker:") != std::string::npos);
  EXPECT_TRUE(output.find("Client ID:") != std::string::npos);
  EXPECT_TRUE(output.find("192.168.1.100") != std::string::npos);
}

TEST_F(LightweightConfigTest, SetDeviceIP) {
  config_->process_command("ip 10.0.0.50");

  auto cfg = config_->config();
  EXPECT_EQ(cfg.device_ip[0], 10);
  EXPECT_EQ(cfg.device_ip[1], 0);
  EXPECT_EQ(cfg.device_ip[2], 0);
  EXPECT_EQ(cfg.device_ip[3], 50);

  std::string output = mock_uart_->get_output();
  EXPECT_TRUE(output.find("Device IP set") != std::string::npos);
}

TEST_F(LightweightConfigTest, SetGateway) {
  config_->process_command("gw 10.0.0.1");

  auto cfg = config_->config();
  EXPECT_EQ(cfg.gateway[0], 10);
  EXPECT_EQ(cfg.gateway[1], 0);
  EXPECT_EQ(cfg.gateway[2], 0);
  EXPECT_EQ(cfg.gateway[3], 1);

  std::string output = mock_uart_->get_output();
  EXPECT_TRUE(output.find("Gateway set") != std::string::npos);
}

TEST_F(LightweightConfigTest, SetBroker) {
  config_->process_command("br 192.168.10.5 8883");

  auto cfg = config_->config();
  EXPECT_EQ(cfg.broker_ip[0], 192);
  EXPECT_EQ(cfg.broker_ip[1], 168);
  EXPECT_EQ(cfg.broker_ip[2], 10);
  EXPECT_EQ(cfg.broker_ip[3], 5);
  EXPECT_EQ(cfg.broker_port, 8883);

  std::string output = mock_uart_->get_output();
  EXPECT_TRUE(output.find("Broker set") != std::string::npos);
}

TEST_F(LightweightConfigTest, SetClientID) {
  config_->process_command("id bell001");

  auto cfg = config_->config();
  EXPECT_STREQ(cfg.client_id, "bell001");

  std::string output = mock_uart_->get_output();
  EXPECT_TRUE(output.find("Client ID set") != std::string::npos);
}

TEST_F(LightweightConfigTest, InvalidIPAddress) {
  mock_uart_->clear_output();
  config_->process_command("ip 999.999.999.999");

  std::string output = mock_uart_->get_output();
  EXPECT_TRUE(output.find("Error") != std::string::npos ||
              output.find("Invalid") != std::string::npos);
}

TEST_F(LightweightConfigTest, InvalidCommand) {
  mock_uart_->clear_output();
  config_->process_command("invalid_cmd");

  std::string output = mock_uart_->get_output();
  EXPECT_TRUE(output.find("Unknown") != std::string::npos ||
              output.find("Error") != std::string::npos);
}

TEST_F(LightweightConfigTest, SaveAndLoadConfig) {
  // Set custom configuration
  config_->process_command("ip 10.20.30.40");
  config_->process_command("gw 10.20.30.1");
  config_->process_command("br 10.20.30.50 1883");
  config_->process_command("id testbell");

  // Save to EEPROM
  config_->save();

  // Create new config instance (should load from EEPROM)
  auto new_config = std::make_unique<Config::LightweightConfig>(*mock_uart_);
  new_config->load();

  auto cfg = new_config->config();

  // Verify loaded values
  EXPECT_EQ(cfg.device_ip[0], 10);
  EXPECT_EQ(cfg.device_ip[1], 20);
  EXPECT_EQ(cfg.device_ip[2], 30);
  EXPECT_EQ(cfg.device_ip[3], 40);

  EXPECT_EQ(cfg.gateway[0], 10);
  EXPECT_EQ(cfg.gateway[1], 20);
  EXPECT_EQ(cfg.gateway[2], 30);
  EXPECT_EQ(cfg.gateway[3], 1);

  EXPECT_EQ(cfg.broker_ip[0], 10);
  EXPECT_EQ(cfg.broker_ip[1], 20);
  EXPECT_EQ(cfg.broker_ip[2], 30);
  EXPECT_EQ(cfg.broker_ip[3], 50);

  EXPECT_STREQ(cfg.client_id, "testbell");
}

TEST_F(LightweightConfigTest, ResetToDefaults) {
  // Set custom configuration
  config_->process_command("ip 10.20.30.40");
  config_->process_command("id testbell");

  // Reset
  config_->process_command("reset");

  auto cfg = config_->config();

  // Should be back to defaults
  EXPECT_EQ(cfg.device_ip[0], 192);
  EXPECT_EQ(cfg.device_ip[1], 168);
  EXPECT_EQ(cfg.device_ip[2], 1);
  EXPECT_EQ(cfg.device_ip[3], 100);

  EXPECT_STREQ(cfg.client_id, "smartbell");
}

TEST_F(LightweightConfigTest, ClientIDMaxLength) {
  // Try to set a very long client ID (should be truncated)
  config_->process_command("id verylongclientidname123456");

  auto cfg = config_->config();
  EXPECT_LE(strlen(cfg.client_id), 15);  // Max 15 chars + null terminator
}

}  // namespace
