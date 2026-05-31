#ifndef PUBLIC_NETWORK_MQTTCLIENT_H_
#define PUBLIC_NETWORK_MQTTCLIENT_H_

#include <stdint.h>
#include "Ethernet/EmbeddedSocketInterface.h"
#include "Serial/Interface.h"

namespace SmartBell {

/**
 * @brief MQTT connection status.
 */
enum class MQTTStatus : uint8_t { kDisconnected = 0, kConnecting, kConnected, kError };

/**
 * @brief MQTT Quality of Service levels.
 */
enum class MQTTQoS : uint8_t {
  kQoS0 = 0,  ///< At most once delivery
  kQoS1 = 1,  ///< At least once delivery
  kQoS2 = 2   ///< Exactly once delivery
};

/**
 * @brief MQTT connection configuration.
 */
struct MQTTConfig {
  uint8_t broker_ip[4];    ///< Broker IP address
  uint16_t broker_port;    ///< Broker port (default 1883)
  const char* client_id;   ///< Client identifier
  uint16_t keepalive_sec;  ///< Keep-alive interval in seconds
  bool clean_session;      ///< Start with clean session
};

/**
 * @brief MQTT authentication credentials.
 */
struct MQTTCredentials {
  const char* username;  ///< Username (can be nullptr)
  const char* password;  ///< Password (can be nullptr)
};

/**
 * @brief MQTT message data for callbacks.
 */
struct MQTTMessageData {
  const char* topic;        ///< Topic string
  uint16_t topic_length;    ///< Topic length
  const uint8_t* payload;   ///< Payload data
  uint16_t payload_length;  ///< Payload length
  MQTTQoS qos;              ///< QoS level
  bool retained;            ///< Retained flag
};

/**
 * @brief Callback type for received MQTT messages.
 */
typedef void (*MQTTMessageCallback)(const MQTTMessageData* message);

/**
 * @brief C++ wrapper for WIZnet ioLibrary MQTT client.
 *
 * Provides MQTT publish/subscribe functionality with optional authentication.
 * Uses TCP socket 2 for MQTT communication.
 * Includes automatic reconnection logic with configurable interval.
 */
class MQTTClient {
 public:
  /// Default socket number for MQTT (TCP socket)
  static constexpr uint8_t kMQTTSocket = 2;

  /// Buffer sizes for MQTT messages
  static constexpr uint16_t kSendBufferSize = 256;
  static constexpr uint16_t kRecvBufferSize = 256;

  /// Default reconnect interval in seconds
  static constexpr uint32_t kDefaultReconnectInterval = 30;

  /// Command timeout in milliseconds
  static constexpr uint32_t kCommandTimeout = 5000;

  /**
   * @brief Construct MQTT client.
   * @param uart Optional UART for debug logging (can be nullptr).
   */
  explicit MQTTClient(serial::Interface* uart = nullptr);
  ~MQTTClient();

  /**
   * @brief Initialize MQTT client.
   * Must be called before connect().
   */
  void init();

  /**
   * @brief Connect to MQTT broker.
   * @param config Connection configuration.
   * @param credentials Optional authentication credentials (can be nullptr).
   * @return true if connection successful.
   */
  bool connect(const MQTTConfig& config, const MQTTCredentials* credentials = nullptr);

  /**
   * @brief Disconnect from MQTT broker.
   */
  void disconnect();

  /**
   * @brief Check if connected to broker.
   * @return true if connected.
   */
  bool is_connected() const;

  /**
   * @brief Get current connection status.
   * @return MQTTStatus enum value.
   */
  MQTTStatus get_status() const;

  /**
   * @brief Publish message to topic.
   * @param topic Topic string.
   * @param payload Message payload.
   * @param length Payload length.
   * @param qos Quality of service level.
   * @param retained Retain message on broker.
   * @return true if publish successful.
   */
  bool publish(const char* topic, const uint8_t* payload, uint16_t length,
               MQTTQoS qos = MQTTQoS::kQoS1, bool retained = false);

  /**
   * @brief Publish string message to topic.
   * @param topic Topic string.
   * @param message Null-terminated message string.
   * @param qos Quality of service level.
   * @return true if publish successful.
   */
  bool publish_string(const char* topic, const char* message, MQTTQoS qos = MQTTQoS::kQoS1);

  /**
   * @brief Subscribe to topic.
   * @param topic Topic filter string.
   * @param callback Function to call when message received.
   * @param qos Quality of service level.
   * @return true if subscription successful.
   */
  bool subscribe(const char* topic, MQTTMessageCallback callback, MQTTQoS qos = MQTTQoS::kQoS1);

  /**
   * @brief Unsubscribe from topic.
   * @param topic Topic filter string.
   * @return true if unsubscribe successful.
   */
  bool unsubscribe(const char* topic);

  /**
   * @brief Process incoming messages and keepalive.
   * Call this regularly in the main loop.
   * @param timeout_ms Yield timeout in milliseconds.
   * @return true if yield completed successfully.
   */
  bool yield(uint32_t timeout_ms = 100);

  /**
   * @brief Set reconnect interval.
   * @param seconds Interval between reconnection attempts.
   */
  void set_reconnect_interval(uint32_t seconds);

  /**
   * @brief Get reconnect interval.
   * @return Interval in seconds.
   */
  uint32_t get_reconnect_interval() const;

  /**
   * @brief Check if reconnect is pending.
   * @return true if waiting for reconnect interval.
   */
  bool is_reconnect_pending() const;

  /**
   * @brief Attempt reconnection if interval has elapsed.
   * @return true if reconnection was attempted.
   */
  bool try_reconnect();

  /**
   * @brief Set callback for message arrival (alternative to per-subscription callbacks).
   * @param callback Default message handler.
   */
  void set_default_message_handler(MQTTMessageCallback callback);

 private:
  serial::Interface* uart_;

  // Buffers for MQTT
  uint8_t send_buffer_[kSendBufferSize];
  uint8_t recv_buffer_[kRecvBufferSize];

  // Connection state
  MQTTStatus status_;
  MQTTConfig last_config_;
  MQTTCredentials last_credentials_;
  bool has_credentials_;

  // Reconnection
  uint32_t reconnect_interval_sec_;
  uint32_t last_disconnect_time_;
  bool reconnect_pending_;

  // Message callback
  MQTTMessageCallback default_callback_;

  // Internal client structures (opaque - allocated on first use)
  void* mqtt_client_;  // MQTTClient*
  void* network_;      // Network*
  bool initialized_;

  /**
   * @brief Log message to UART if available.
   * @param message Message to log.
   */
  void log(const char* message);

  /**
   * @brief Log IP address to UART.
   * @param label Label for the IP.
   * @param ip IP address bytes.
   */
  void log_ip(const char* label, const uint8_t* ip);

 public:
  /**
   * @brief Handle incoming message (called from internal handler).
   * @param message_data Pointer to MessageData from ioLibrary.
   */
  void handle_message(void* message_data);

  // Static instance for callbacks (public for free function access)
  static MQTTClient* instance_;
};

}  // namespace SmartBell

#endif  // PUBLIC_NETWORK_MQTTCLIENT_H_
