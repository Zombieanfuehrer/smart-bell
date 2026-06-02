#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Include hardware mock before any AVR headers
#include "../Mocks/AVRHardwareMock.h"

#include "MQTT/MinimalMQTT.h"
#include "System/TimerService.h"

// Mock for W5500 Socket API
extern "C" {
// Socket API function pointers that will be mocked
static int8_t (*mock_socket_fn)(uint8_t sn, uint8_t protocol, uint16_t port,
                                uint8_t flag) = nullptr;
static int8_t (*mock_connect_fn)(uint8_t sn, uint8_t* addr, uint16_t port) = nullptr;
static int32_t (*mock_send_fn)(uint8_t sn, uint8_t* buf, uint16_t len) = nullptr;
static int32_t (*mock_recv_fn)(uint8_t sn, uint8_t* buf, uint16_t len) = nullptr;
static int8_t (*mock_disconnect_fn)(uint8_t sn) = nullptr;
static int8_t (*mock_close_fn)(uint8_t sn) = nullptr;
static uint8_t (*mock_getSn_SR_fn)(uint8_t sn) = nullptr;
static uint16_t (*mock_getSn_RX_RSR_fn)(uint8_t sn) = nullptr;

// Mock implementations
int8_t socket(uint8_t sn, uint8_t protocol, uint16_t port, uint8_t flag) {
  return mock_socket_fn ? mock_socket_fn(sn, protocol, port, flag) : -1;
}

int8_t connect(uint8_t sn, uint8_t* addr, uint16_t port) {
  return mock_connect_fn ? mock_connect_fn(sn, addr, port) : -1;
}

int32_t send(uint8_t sn, uint8_t* buf, uint16_t len) {
  return mock_send_fn ? mock_send_fn(sn, buf, len) : -1;
}

int32_t recv(uint8_t sn, uint8_t* buf, uint16_t len) {
  return mock_recv_fn ? mock_recv_fn(sn, buf, len) : -1;
}

int8_t disconnect(uint8_t sn) { return mock_disconnect_fn ? mock_disconnect_fn(sn) : -1; }

int8_t closesocket(uint8_t sn) { return mock_close_fn ? mock_close_fn(sn) : -1; }

uint8_t getSn_SR(uint8_t sn) { return mock_getSn_SR_fn ? mock_getSn_SR_fn(sn) : 0; }

uint16_t getSn_RX_RSR(uint8_t sn) { return mock_getSn_RX_RSR_fn ? mock_getSn_RX_RSR_fn(sn) : 0; }
}

// Mock TimerService
class MockTimerService : public System::ITimerService {
 public:
  uint32_t get_millis() const override { return current_millis_; }

  void set_millis(uint32_t millis) { current_millis_ = millis; }

  void advance_time(uint32_t delta) { current_millis_ += delta; }

 private:
  uint32_t current_millis_ = 0;
};

class MinimalMQTTTest : public ::testing::Test {
 protected:
  void SetUp() override {
    timer_service_ = std::make_unique<MockTimerService>();

    // Setup mock functions with default behaviors
    mock_socket_fn = [](uint8_t, uint8_t, uint16_t, uint8_t) -> int8_t { return 2; };
    mock_connect_fn = [](uint8_t, uint8_t*, uint16_t) -> int8_t { return 0; };
    mock_send_fn = [](uint8_t, uint8_t*, uint16_t len) -> int32_t { return len; };
    mock_recv_fn = [](uint8_t, uint8_t*, uint16_t) -> int32_t { return 0; };
    mock_disconnect_fn = [](uint8_t) -> int8_t { return 0; };
    mock_close_fn = [](uint8_t) -> int8_t { return 0; };
    mock_getSn_SR_fn = [](uint8_t) -> uint8_t { return 0x17; };  // SOCK_ESTABLISHED
    mock_getSn_RX_RSR_fn = [](uint8_t) -> uint16_t { return 0; };

    // Setup MQTT config
    config_.broker_ip[0] = 192;
    config_.broker_ip[1] = 168;
    config_.broker_ip[2] = 1;
    config_.broker_ip[3] = 10;
    config_.broker_port = 1883;
    strncpy(config_.client_id, "test_client", sizeof(config_.client_id) - 1);
    config_.keepalive = 60;
    config_.use_auth = false;

    mqtt_ = std::make_unique<MQTT::MinimalMQTT>(*timer_service_);
  }

  void TearDown() override {
    mqtt_.reset();
    timer_service_.reset();

    // Clear mock function pointers
    mock_socket_fn = nullptr;
    mock_connect_fn = nullptr;
    mock_send_fn = nullptr;
    mock_recv_fn = nullptr;
    mock_disconnect_fn = nullptr;
    mock_close_fn = nullptr;
    mock_getSn_SR_fn = nullptr;
    mock_getSn_RX_RSR_fn = nullptr;
  }

  std::unique_ptr<MockTimerService> timer_service_;
  std::unique_ptr<MQTT::MinimalMQTT> mqtt_;
  MQTT::MinimalMQTT::Config config_;
};

TEST_F(MinimalMQTTTest, InitialState) { EXPECT_FALSE(mqtt_->is_connected()); }

TEST_F(MinimalMQTTTest, ConnectSuccess) {
  bool connect_result = mqtt_->connect(config_);
  EXPECT_TRUE(connect_result);
  EXPECT_TRUE(mqtt_->is_connected());
}

TEST_F(MinimalMQTTTest, ConnectFailure_SocketError) {
  mock_socket_fn = [](uint8_t, uint8_t, uint16_t, uint8_t) -> int8_t { return -1; };

  bool connect_result = mqtt_->connect(config_);
  EXPECT_FALSE(connect_result);
  EXPECT_FALSE(mqtt_->is_connected());
}

TEST_F(MinimalMQTTTest, ConnectFailure_ConnectionRefused) {
  mock_connect_fn = [](uint8_t, uint8_t*, uint16_t) -> int8_t { return -1; };

  bool connect_result = mqtt_->connect(config_);
  EXPECT_FALSE(connect_result);
  EXPECT_FALSE(mqtt_->is_connected());
}

TEST_F(MinimalMQTTTest, PublishMessage) {
  mqtt_->connect(config_);

  // Mock send to capture published data
  std::vector<uint8_t> sent_data;
  mock_send_fn = [&sent_data](uint8_t, uint8_t* buf, uint16_t len) -> int32_t {
    sent_data.insert(sent_data.end(), buf, buf + len);
    return len;
  };

  bool pub_result = mqtt_->publish("test/topic", "Hello MQTT", false);
  EXPECT_TRUE(pub_result);

  // Verify PUBLISH packet was sent (starts with 0x30 for QoS 0)
  ASSERT_GT(sent_data.size(), 0);
  EXPECT_EQ(sent_data[0] & 0xF0, 0x30);  // MQTT PUBLISH packet type
}

TEST_F(MinimalMQTTTest, PublishWithRetain) {
  mqtt_->connect(config_);

  std::vector<uint8_t> sent_data;
  mock_send_fn = [&sent_data](uint8_t, uint8_t* buf, uint16_t len) -> int32_t {
    sent_data.insert(sent_data.end(), buf, buf + len);
    return len;
  };

  bool pub_result = mqtt_->publish("test/topic", "retained", true);
  EXPECT_TRUE(pub_result);

  // Verify RETAIN flag is set (bit 0 of first byte)
  ASSERT_GT(sent_data.size(), 0);
  EXPECT_TRUE(sent_data[0] & 0x01);
}

TEST_F(MinimalMQTTTest, PublishFailure_NotConnected) {
  // Don't connect
  bool pub_result = mqtt_->publish("test/topic", "message", false);
  EXPECT_FALSE(pub_result);
}

TEST_F(MinimalMQTTTest, PublishFailure_TopicTooLong) {
  mqtt_->connect(config_);

  // Create topic longer than kMaxTopicLength (32)
  std::string long_topic(40, 'a');
  bool pub_result = mqtt_->publish(long_topic.c_str(), "message", false);
  EXPECT_FALSE(pub_result);
}

TEST_F(MinimalMQTTTest, Subscribe) {
  mqtt_->connect(config_);

  std::vector<uint8_t> sent_data;
  mock_send_fn = [&sent_data](uint8_t, uint8_t* buf, uint16_t len) -> int32_t {
    sent_data.insert(sent_data.end(), buf, buf + len);
    return len;
  };

  auto callback = [](const char*, const uint8_t*, uint16_t) {};
  bool sub_result = mqtt_->subscribe("sensors/#", callback);
  EXPECT_TRUE(sub_result);

  // Verify SUBSCRIBE packet was sent (0x82)
  ASSERT_GT(sent_data.size(), 0);
  EXPECT_EQ(sent_data[0], 0x82);
}

TEST_F(MinimalMQTTTest, SubscribeFailure_MaxSubscriptionsReached) {
  mqtt_->connect(config_);

  auto callback = [](const char*, const uint8_t*, uint16_t) {};

  // First subscription should succeed
  EXPECT_TRUE(mqtt_->subscribe("topic1", callback));

  // Second subscription should fail (max 1 subscription)
  EXPECT_FALSE(mqtt_->subscribe("topic2", callback));
}

TEST_F(MinimalMQTTTest, KeepaliveLoop) {
  mqtt_->connect(config_);

  // Set keepalive to 60 seconds, so PINGREQ at 45s (75%)
  config_.keepalive = 60;

  std::vector<uint8_t> sent_data;
  int send_count = 0;
  mock_send_fn = [&sent_data, &send_count](uint8_t, uint8_t* buf, uint16_t len) -> int32_t {
    if (send_count > 0) {  // Skip initial CONNECT packet
      sent_data.clear();
      sent_data.insert(sent_data.end(), buf, buf + len);
    }
    send_count++;
    return len;
  };

  // Advance time to trigger keepalive (45 seconds)
  timer_service_->advance_time(45000);
  mqtt_->loop();

  // Verify PINGREQ was sent (0xC0)
  ASSERT_GT(sent_data.size(), 0);
  EXPECT_EQ(sent_data[0], 0xC0);
}

TEST_F(MinimalMQTTTest, DisconnectCleanly) {
  mqtt_->connect(config_);
  EXPECT_TRUE(mqtt_->is_connected());

  mqtt_->disconnect();
  EXPECT_FALSE(mqtt_->is_connected());
}

TEST_F(MinimalMQTTTest, MessageCallback) {
  mqtt_->connect(config_);

  std::string received_topic;
  std::string received_payload;

  auto callback = [&](const char* topic, const uint8_t* payload, uint16_t length) {
    received_topic = topic;
    received_payload = std::string(reinterpret_cast<const char*>(payload), length);
  };

  mqtt_->subscribe("test/callback", callback);

  // Simulate receiving a PUBLISH packet
  const char* test_topic = "test/callback";
  const char* test_payload = "Hello";

  // Build MQTT PUBLISH packet manually
  std::vector<uint8_t> packet;
  packet.push_back(0x30);  // PUBLISH QoS 0

  // Remaining length calculation
  uint16_t remaining = 2 + strlen(test_topic) + strlen(test_payload);
  packet.push_back(static_cast<uint8_t>(remaining));

  // Topic length (MSB, LSB)
  uint16_t topic_len = strlen(test_topic);
  packet.push_back(static_cast<uint8_t>(topic_len >> 8));
  packet.push_back(static_cast<uint8_t>(topic_len & 0xFF));

  // Topic
  for (size_t i = 0; i < topic_len; i++) {
    packet.push_back(test_topic[i]);
  }

  // Payload
  for (size_t i = 0; i < strlen(test_payload); i++) {
    packet.push_back(test_payload[i]);
  }

  // Mock recv to return our packet
  size_t recv_pos = 0;
  mock_getSn_RX_RSR_fn = [&](uint8_t) -> uint16_t {
    return (recv_pos < packet.size()) ? (packet.size() - recv_pos) : 0;
  };

  mock_recv_fn = [&](uint8_t, uint8_t* buf, uint16_t len) -> int32_t {
    uint16_t to_copy = std::min(static_cast<uint16_t>(packet.size() - recv_pos), len);
    memcpy(buf, packet.data() + recv_pos, to_copy);
    recv_pos += to_copy;
    return to_copy;
  };

  // Process received message
  mqtt_->loop();

  // Verify callback was called with correct data
  EXPECT_EQ(received_topic, "test/callback");
  EXPECT_EQ(received_payload, "Hello");
}

}  // namespace
