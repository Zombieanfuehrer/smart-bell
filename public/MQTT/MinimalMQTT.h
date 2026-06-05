#ifndef PUBLIC_MQTT_MINIMALMQTT_H_
#define PUBLIC_MQTT_MINIMALMQTT_H_

#include <stdint.h>
#include "Serial/UART.h"

namespace MQTT {

// MQTT Protocol Constants
constexpr uint8_t kMQTTProtocolLevel = 4;   // MQTT 3.1.1
constexpr uint16_t kDefaultKeepalive = 60;  // seconds

// Buffer sizes - optimized for ATmega328P
constexpr uint16_t kSendBufferSize = 64;
constexpr uint16_t kRecvBufferSize = 64;
constexpr uint8_t kMaxTopicLength = 32;
constexpr uint8_t kMaxClientIdLength = 24;
constexpr uint8_t kMaxUsernameLength = 24;
constexpr uint8_t kMaxPasswordLength = 24;
constexpr uint8_t kMaxSubscriptions = 2;

// MQTT Message Types
enum class MessageType : uint8_t {
  CONNECT = 0x10,
  CONNACK = 0x20,
  PUBLISH = 0x30,
  SUBSCRIBE = 0x82,
  SUBACK = 0x90,
  PINGREQ = 0xC0,
  PINGRESP = 0xD0,
  DISCONNECT = 0xE0
};

// Connection states
enum class State : uint8_t { DISCONNECTED, CONNECTING, CONNECTED, ERROR };

// Message callback for subscriptions
using MessageCallback = void (*)(const char* topic, const uint8_t* payload, uint16_t length);

// Configuration structure - stored in SRAM
struct Config {
  uint8_t broker_ip[4];
  uint16_t broker_port;
  char client_id[kMaxClientIdLength];
  char username[kMaxUsernameLength];
  char password[kMaxPasswordLength];
  uint16_t keepalive;
  bool use_auth;
};

// Subscription entry
struct Subscription {
  char topic[kMaxTopicLength];
  MessageCallback callback;
  bool active;
};

/**
 * @brief Minimal MQTT 3.1.1 client for W5500 on ATmega328P.
 *
 * Design constraints:
 * - No dynamic allocation (no new/delete/malloc)
 * - Fixed-size static buffers
 * - QoS 0 only (fire-and-forget)
 * - Uses W5500 socket 2 for MQTT
 * - Minimal feature set for SmartBell use case
 */
class MinimalMQTT {
 public:
  static constexpr uint8_t kMQTTSocketNumber = 2;

  /**
   * @brief Construct MQTT client.
   * @param uart Optional UART for debug logging (nullptr to disable).
   */
  explicit MinimalMQTT(serial::UART* uart = nullptr);
  ~MinimalMQTT() = default;

  // Prevent copying
  MinimalMQTT(const MinimalMQTT&) = delete;
  MinimalMQTT& operator=(const MinimalMQTT&) = delete;

  /**
   * @brief Connect to MQTT broker.
   * @param config Connection configuration.
   * @return true if connection successful.
   */
  bool connect(const Config& config);

  /**
   * @brief Disconnect from broker.
   */
  void disconnect();

  /**
   * @brief Check if connected.
   */
  bool is_connected() const { return state_ == State::CONNECTED; }

  /**
   * @brief Get current state.
   */
  State get_state() const { return state_; }

  /**
   * @brief Publish message (QoS 0).
   * @param topic Topic string (max 47 chars).
   * @param payload Payload data.
   * @param length Payload length.
   * @return true if sent successfully.
   */
  bool publish(const char* topic, const uint8_t* payload, uint16_t length);

  /**
   * @brief Publish string message (QoS 0).
   */
  bool publish_string(const char* topic, const char* message);

  /**
   * @brief Subscribe to topic.
   * @param topic Topic string (max 47 chars).
   * @param callback Message callback.
   * @return true if subscription sent.
   */
  bool subscribe(const char* topic, MessageCallback callback);

  /**
   * @brief Process incoming messages and handle keepalive.
   * Must be called regularly (at least every second).
   */
  void loop();

 private:
  // Socket operations
  bool socket_connect();
  void socket_disconnect();
  bool socket_send(const uint8_t* data, uint16_t length);
  int16_t socket_recv(uint8_t* buffer, uint16_t max_length);
  bool socket_is_connected();

  // MQTT protocol
  bool send_connect_packet();
  bool wait_for_connack();
  void send_pingreq();
  void process_incoming_packet();

  // Packet building helpers
  uint16_t encode_string(uint8_t* buffer, const char* str);
  uint16_t encode_remaining_length(uint8_t* buffer, uint16_t length);

  // Logging helper
  void log(const char* message);

  // State
  serial::UART* uart_;
  State state_;
  Config config_;

  // Buffers (statically allocated)
  uint8_t send_buffer_[kSendBufferSize];
  uint8_t recv_buffer_[kRecvBufferSize];

  // Subscriptions
  Subscription subscriptions_[kMaxSubscriptions];

  // Timing
  uint32_t last_activity_;  // millis() of last send/recv
  uint32_t last_ping_;      // millis() of last PINGREQ

  // Packet ID counter
  uint16_t packet_id_;
};

}  // namespace MQTT

#endif  // PUBLIC_MQTT_MINIMALMQTT_H_
