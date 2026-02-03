#ifndef PUBLIC_MQTT_MQTTCLIENT_H_
#define PUBLIC_MQTT_MQTTCLIENT_H_

#include <stdint.h>
#include <stdbool.h>

#include "Ethernet/EmbeddedSocketInterface.h"
#include "Serial/UART.h"

namespace MQTT {

// MQTT Protocol Constants
constexpr uint8_t MQTT_PROTOCOL_LEVEL = 4; // MQTT 3.1.1
constexpr uint16_t MQTT_DEFAULT_KEEPALIVE = 60; // seconds
constexpr uint16_t MQTT_MAX_PACKET_SIZE = 256;
constexpr uint16_t MQTT_MAX_PAYLOAD_SIZE = 128;
constexpr uint8_t MQTT_MAX_TOPICS = 4;

// MQTT Message Types
enum class MQTTMessageType : uint8_t {
  CONNECT = 1,
  CONNACK = 2,
  PUBLISH = 3,
  PUBACK = 4,
  SUBSCRIBE = 8,
  SUBACK = 9,
  PINGREQ = 12,
  PINGRESP = 13,
  DISCONNECT = 14
};

// MQTT QoS Levels
enum class MQTTQoS : uint8_t {
  QoS0 = 0, // At most once
  QoS1 = 1, // At least once
  QoS2 = 2  // Exactly once
};

// MQTT Connection States
enum class MQTTConnectionState : uint8_t {
  DISCONNECTED = 0,
  CONNECTING = 1,
  CONNECTED = 2,
  RECONNECTING = 3
};

// MQTT Connection Result
enum class MQTTConnectResult : uint8_t {
  ACCEPTED = 0,
  REFUSED_PROTOCOL = 1,
  REFUSED_IDENTIFIER = 2,
  REFUSED_SERVER_UNAVAILABLE = 3,
  REFUSED_BAD_CREDENTIALS = 4,
  REFUSED_NOT_AUTHORIZED = 5,
  ERROR_NETWORK = 6,
  ERROR_TIMEOUT = 7
};

// Configuration structure
struct MQTTConfig {
  char broker_hostname[64];
  uint16_t broker_port;
  char client_id[32];
  char username[32];
  char password[32];
  uint16_t keepalive;
  uint16_t reconnect_interval; // seconds
};

// Topic subscription callback
using MQTTMessageCallback = void (*)(const char* topic, const uint8_t* payload, uint16_t length);

// Subscription structure
struct MQTTSubscription {
  char topic[48];
  MQTTQoS qos;
  MQTTMessageCallback callback;
  bool active;
};

class MQTTClient {
 public:
  MQTTClient(Ethernet::EmbeddedSocketW5500* socket_manager, 
             serial::UART* uart_log);
  ~MQTTClient() = default;

  // Connection management
  bool connect(const MQTTConfig& config);
  void disconnect();
  bool reconnect();
  
  // State management
  void loop(); // Must be called regularly
  MQTTConnectionState get_state() const { return state_; }
  bool is_connected() const { return state_ == MQTTConnectionState::CONNECTED; }

  // Publishing
  bool publish(const char* topic, const uint8_t* payload, uint16_t length, 
               MQTTQoS qos = MQTTQoS::QoS0, bool retain = false);
  bool publish(const char* topic, const char* payload, 
               MQTTQoS qos = MQTTQoS::QoS0, bool retain = false);

  // Subscribing
  bool subscribe(const char* topic, MQTTQoS qos, MQTTMessageCallback callback);
  bool unsubscribe(const char* topic);

 private:
  // Internal connection handling
  bool connect_to_broker();
  bool send_connect_packet();
  bool wait_for_connack();
  void handle_disconnect();

  // Packet handling
  bool send_packet(const uint8_t* buffer, uint16_t length);
  bool receive_packet(uint8_t* buffer, uint16_t* length);
  void process_incoming_packets();
  
  // Specific packet handlers
  void handle_publish(const uint8_t* buffer, uint16_t length);
  void handle_connack(const uint8_t* buffer, uint16_t length);
  void handle_suback(const uint8_t* buffer, uint16_t length);
  void handle_pingresp();

  // Protocol helpers
  uint16_t encode_remaining_length(uint8_t* buffer, uint16_t length);
  uint16_t decode_remaining_length(const uint8_t* buffer, uint16_t* value);
  uint16_t encode_string(uint8_t* buffer, const char* str);
  uint16_t decode_string(const uint8_t* buffer, char* str, uint16_t max_len);

  // Keepalive management
  void send_ping();
  void update_keepalive();

  // Member variables
  Ethernet::EmbeddedSocketW5500* socket_manager_;
  serial::UART* uart_log_;
  
  MQTTConfig config_;
  MQTTConnectionState state_;
  uint8_t socket_number_;
  
  uint16_t packet_id_;
  uint32_t last_activity_; // milliseconds
  uint32_t last_ping_; // milliseconds
  uint32_t last_reconnect_attempt_; // milliseconds
  
  MQTTSubscription subscriptions_[MQTT_MAX_TOPICS];
  uint8_t subscription_count_;
  
  uint8_t tx_buffer_[MQTT_MAX_PACKET_SIZE];
  uint8_t rx_buffer_[MQTT_MAX_PACKET_SIZE];
  
  bool pending_ping_;
  MQTTConnectResult last_connect_result_;
};

} // namespace MQTT

#endif // PUBLIC_MQTT_MQTTCLIENT_H_
