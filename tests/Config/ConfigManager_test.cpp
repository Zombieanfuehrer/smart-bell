#include "Config/ConfigManager.h"
#include <gtest/gtest.h>
#include <cstring>

using namespace Config;

class ConfigManagerTest : public ::testing::Test {
 protected:
  void SetUp() override { config_manager_ = new ConfigManager(); }

  void TearDown() override { delete config_manager_; }

  ConfigManager* config_manager_;
};

// Test default initialization
TEST_F(ConfigManagerTest, DefaultsAreSet) {
  EXPECT_STREQ(config_manager_->get_client_id(), "smartbell");
  EXPECT_EQ(config_manager_->get_port(), 1883);
  EXPECT_STREQ(config_manager_->get_broker(), "");
  EXPECT_FALSE(config_manager_->is_valid());
}

// Test broker IP address detection
TEST_F(ConfigManagerTest, DetectsIPAddress) {
  config_manager_->set_broker("192.168.1.100");
  EXPECT_FALSE(config_manager_->broker_is_hostname());

  config_manager_->set_broker("10.0.0.1");
  EXPECT_FALSE(config_manager_->broker_is_hostname());

  config_manager_->set_broker("255.255.255.255");
  EXPECT_FALSE(config_manager_->broker_is_hostname());
}

// Test broker hostname detection
TEST_F(ConfigManagerTest, DetectsHostname) {
  config_manager_->set_broker("mqtt.example.com");
  EXPECT_TRUE(config_manager_->broker_is_hostname());

  config_manager_->set_broker("localhost");
  EXPECT_TRUE(config_manager_->broker_is_hostname());

  config_manager_->set_broker("my-broker.local");
  EXPECT_TRUE(config_manager_->broker_is_hostname());
}

// Test IP address parsing
TEST_F(ConfigManagerTest, ParsesIPAddress) {
  config_manager_->set_broker("192.168.1.100");

  uint8_t ip[4];
  EXPECT_TRUE(config_manager_->get_broker_ip(ip));
  EXPECT_EQ(ip[0], 192);
  EXPECT_EQ(ip[1], 168);
  EXPECT_EQ(ip[2], 1);
  EXPECT_EQ(ip[3], 100);
}

// Test IP parsing with different values
TEST_F(ConfigManagerTest, ParsesVariousIPAddresses) {
  uint8_t ip[4];

  config_manager_->set_broker("10.0.0.1");
  EXPECT_TRUE(config_manager_->get_broker_ip(ip));
  EXPECT_EQ(ip[0], 10);
  EXPECT_EQ(ip[1], 0);
  EXPECT_EQ(ip[2], 0);
  EXPECT_EQ(ip[3], 1);

  config_manager_->set_broker("255.255.255.0");
  EXPECT_TRUE(config_manager_->get_broker_ip(ip));
  EXPECT_EQ(ip[0], 255);
  EXPECT_EQ(ip[1], 255);
  EXPECT_EQ(ip[2], 255);
  EXPECT_EQ(ip[3], 0);
}

// Test hostname returns false for get_broker_ip
TEST_F(ConfigManagerTest, HostnameReturnsNoIP) {
  config_manager_->set_broker("mqtt.local");

  uint8_t ip[4] = {0, 0, 0, 0};
  EXPECT_FALSE(config_manager_->get_broker_ip(ip));
}

// Test port setting
TEST_F(ConfigManagerTest, SetsPort) {
  config_manager_->set_port(8883);
  EXPECT_EQ(config_manager_->get_port(), 8883);

  config_manager_->set_port(1883);
  EXPECT_EQ(config_manager_->get_port(), 1883);
}

// Test credentials
TEST_F(ConfigManagerTest, SetsCredentials) {
  config_manager_->set_username("testuser");
  config_manager_->set_password("testpass");

  EXPECT_STREQ(config_manager_->get_username(), "testuser");
  EXPECT_STREQ(config_manager_->get_password(), "testpass");
}

// Test client ID
TEST_F(ConfigManagerTest, SetsClientId) {
  config_manager_->set_client_id("bell-01");
  EXPECT_STREQ(config_manager_->get_client_id(), "bell-01");
}

// Test validity requires broker
TEST_F(ConfigManagerTest, ValidityRequiresBroker) {
  // Without broker, not valid
  EXPECT_FALSE(config_manager_->is_valid());

  // With broker but no save, still not valid (need to save on x86 mock)
  config_manager_->set_broker("192.168.1.100");
  config_manager_->save();  // This marks as valid on x86

  EXPECT_TRUE(config_manager_->is_valid());
}

// Test reset to defaults
TEST_F(ConfigManagerTest, ResetToDefaults) {
  config_manager_->set_broker("test.example.com");
  config_manager_->set_port(9999);
  config_manager_->set_username("user");
  config_manager_->set_password("pass");

  config_manager_->reset_to_defaults();

  EXPECT_STREQ(config_manager_->get_broker(), "");
  EXPECT_EQ(config_manager_->get_port(), 1883);
  EXPECT_STREQ(config_manager_->get_username(), "");
  EXPECT_STREQ(config_manager_->get_password(), "");
  EXPECT_FALSE(config_manager_->is_valid());
}

// Test string truncation
TEST_F(ConfigManagerTest, TruncatesLongStrings) {
  // Create a very long string
  char long_broker[128];
  memset(long_broker, 'a', 127);
  long_broker[127] = '\0';

  config_manager_->set_broker(long_broker);

  // Should be truncated to kMaxBrokerLength - 1
  EXPECT_LT(strlen(config_manager_->get_broker()), 128);
}

// Test runtime config structure
TEST_F(ConfigManagerTest, RuntimeConfigIsUpdated) {
  config_manager_->set_broker("192.168.1.50");
  config_manager_->set_port(1884);
  config_manager_->set_username("mqttuser");
  config_manager_->save();

  const RuntimeConfig& rc = config_manager_->get_runtime_config();

  EXPECT_STREQ(rc.broker, "192.168.1.50");
  EXPECT_EQ(rc.broker_port, 1884);
  EXPECT_STREQ(rc.username, "mqttuser");
  EXPECT_FALSE(rc.broker_is_hostname);
  EXPECT_TRUE(rc.is_valid);
}
