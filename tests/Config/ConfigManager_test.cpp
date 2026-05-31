#include <gtest/gtest.h>
#include "Config/ConfigManager.h"
#include <cstring>

// Mock UART for testing
class MockUART : public serial::Interface {
 public:
  void send(const uint8_t byte) override {
    output += static_cast<char>(byte);
  }
  
  void send_bytes(const uint8_t *const bytes, const uint16_t length) override {
    for (uint16_t i = 0; i < length; i++) {
      output += static_cast<char>(bytes[i]);
    }
  }
  
  void send_string(const char *string) override {
    output += string;
  }
  
  bool is_read_data_available() const override {
    return false;
  }
  
  uint8_t read_byte() override {
    return 0;
  }
  
  std::string output;
  
  void clear_output() {
    output.clear();
  }
};

class ConfigManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mock_uart = new MockUART();
    config_manager = new Config::ConfigManager(reinterpret_cast<serial::UART*>(mock_uart));
  }
  
  void TearDown() override {
    delete config_manager;
    delete mock_uart;
  }
  
  MockUART* mock_uart;
  Config::ConfigManager* config_manager;
};

TEST_F(ConfigManagerTest, LoadDefaults) {
  config_manager->load_defaults();
  const Config::SmartBellConfig& config = config_manager->get_config();
  
  EXPECT_STREQ(config.mqtt_broker, "192.168.1.100");
  EXPECT_EQ(config.mqtt_port, 1883);
  EXPECT_STREQ(config.mqtt_client_id, "smart-bell");
  EXPECT_TRUE(config.use_dhcp);
  EXPECT_EQ(config.ring_duration, 6);
  EXPECT_EQ(config.reconnect_interval, 30);
  EXPECT_EQ(config.magic_number, 0xBEEFCAFE);
}

TEST_F(ConfigManagerTest, SetMQTTBroker) {
  config_manager->set_mqtt_broker("192.168.1.50", 1884);
  const Config::SmartBellConfig& config = config_manager->get_config();
  
  EXPECT_STREQ(config.mqtt_broker, "192.168.1.50");
  EXPECT_EQ(config.mqtt_port, 1884);
}

TEST_F(ConfigManagerTest, SetMQTTCredentials) {
  config_manager->set_mqtt_credentials("my-bell", "user123", "pass456");
  const Config::SmartBellConfig& config = config_manager->get_config();
  
  EXPECT_STREQ(config.mqtt_client_id, "my-bell");
  EXPECT_STREQ(config.mqtt_username, "user123");
  EXPECT_STREQ(config.mqtt_password, "pass456");
}

TEST_F(ConfigManagerTest, SetMQTTTopics) {
  config_manager->set_mqtt_topics("test/ring", "test/ctrl");
  const Config::SmartBellConfig& config = config_manager->get_config();
  
  EXPECT_STREQ(config.mqtt_topic_ring, "test/ring");
  EXPECT_STREQ(config.mqtt_topic_control, "test/ctrl");
}

TEST_F(ConfigManagerTest, SetDHCP) {
  config_manager->set_dhcp(false);
  const Config::SmartBellConfig& config = config_manager->get_config();
  EXPECT_FALSE(config.use_dhcp);
  
  config_manager->set_dhcp(true);
  EXPECT_TRUE(config.use_dhcp);
}

TEST_F(ConfigManagerTest, GetMQTTConfig) {
  config_manager->load_defaults();
  config_manager->set_mqtt_credentials("test-id", "testuser", "testpass");
  
  MQTT::MQTTConfig mqtt_config;
  config_manager->get_mqtt_config(mqtt_config);
  
  EXPECT_STREQ(mqtt_config.broker_hostname, "192.168.1.100");
  EXPECT_EQ(mqtt_config.broker_port, 1883);
  EXPECT_STREQ(mqtt_config.client_id, "test-id");
  EXPECT_STREQ(mqtt_config.username, "testuser");
  EXPECT_STREQ(mqtt_config.password, "testpass");
  EXPECT_EQ(mqtt_config.keepalive, 60);
  EXPECT_EQ(mqtt_config.reconnect_interval, 30);
}

TEST_F(ConfigManagerTest, ProcessShowConfigCommand) {
  mock_uart->clear_output();
  bool result = config_manager->process_uart_command("config show");
  
  EXPECT_TRUE(result);
  EXPECT_TRUE(mock_uart->output.find("Smart Bell Configuration") != std::string::npos);
  EXPECT_TRUE(mock_uart->output.find("MQTT Broker") != std::string::npos);
}

TEST_F(ConfigManagerTest, ProcessSetBrokerCommand) {
  mock_uart->clear_output();
  bool result = config_manager->process_uart_command("set broker 192.168.2.50 1884");
  
  EXPECT_TRUE(result);
  const Config::SmartBellConfig& config = config_manager->get_config();
  EXPECT_STREQ(config.mqtt_broker, "192.168.2.50");
  EXPECT_EQ(config.mqtt_port, 1884);
}

TEST_F(ConfigManagerTest, ProcessSetCredentialsCommand) {
  mock_uart->clear_output();
  bool result = config_manager->process_uart_command("set cred mybell myuser mypass");
  
  EXPECT_TRUE(result);
  const Config::SmartBellConfig& config = config_manager->get_config();
  EXPECT_STREQ(config.mqtt_client_id, "mybell");
  EXPECT_STREQ(config.mqtt_username, "myuser");
  EXPECT_STREQ(config.mqtt_password, "mypass");
}

TEST_F(ConfigManagerTest, ProcessHelpCommand) {
  mock_uart->clear_output();
  bool result = config_manager->process_uart_command("help");
  
  EXPECT_TRUE(result);
  EXPECT_TRUE(mock_uart->output.find("Available commands") != std::string::npos);
  EXPECT_TRUE(mock_uart->output.find("config show") != std::string::npos);
  EXPECT_TRUE(mock_uart->output.find("set broker") != std::string::npos);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
