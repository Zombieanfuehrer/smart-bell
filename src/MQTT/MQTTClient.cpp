#include "MQTT/MQTTClient.h"

#ifdef __AVR__
#include <avr/io.h>
#include <util/delay.h>
#endif

#include <string.h>

namespace MQTT {

// Helper function to get current time in milliseconds (simplified for AVR)
static uint32_t millis() {
  // This is a placeholder - in real implementation, use timer
  static uint32_t ms_counter = 0;
  return ms_counter++;
}

MQTTClient::MQTTClient(Ethernet::EmbeddedSocketW5500* socket_manager, 
                       serial::UART* uart_log)
    : socket_manager_(socket_manager),
      uart_log_(uart_log),
      state_(MQTTConnectionState::DISCONNECTED),
      socket_number_(1), // Use socket 1 for MQTT
      packet_id_(1),
      last_activity_(0),
      last_ping_(0),
      last_reconnect_attempt_(0),
      subscription_count_(0),
      pending_ping_(false),
      last_connect_result_(MQTTConnectResult::ACCEPTED) {
  
  // Initialize subscriptions
  for (uint8_t i = 0; i < MQTT_MAX_TOPICS; i++) {
    subscriptions_[i].active = false;
    subscriptions_[i].callback = nullptr;
  }
  
  // Initialize configuration
  memset(&config_, 0, sizeof(config_));
  config_.broker_port = 1883;
  config_.keepalive = MQTT_DEFAULT_KEEPALIVE;
  config_.reconnect_interval = 30;
}

bool MQTTClient::connect(const MQTTConfig& config) {
  if (uart_log_) {
    uart_log_->send_string("[MQTT] Connecting to broker...\r\n");
  }
  
  // Copy configuration
  memcpy(&config_, &config, sizeof(MQTTConfig));
  
  state_ = MQTTConnectionState::CONNECTING;
  
  // Connect to broker
  if (!connect_to_broker()) {
    state_ = MQTTConnectionState::DISCONNECTED;
    if (uart_log_) {
      uart_log_->send_string("[MQTT] Failed to connect to broker\r\n");
    }
    return false;
  }
  
  // Send CONNECT packet
  if (!send_connect_packet()) {
    state_ = MQTTConnectionState::DISCONNECTED;
    if (uart_log_) {
      uart_log_->send_string("[MQTT] Failed to send CONNECT packet\r\n");
    }
    return false;
  }
  
  // Wait for CONNACK
  if (!wait_for_connack()) {
    state_ = MQTTConnectionState::DISCONNECTED;
    if (uart_log_) {
      uart_log_->send_string("[MQTT] Failed to receive CONNACK\r\n");
    }
    return false;
  }
  
  state_ = MQTTConnectionState::CONNECTED;
  last_activity_ = millis();
  last_ping_ = millis();
  
  if (uart_log_) {
    uart_log_->send_string("[MQTT] Connected successfully\r\n");
  }
  
  return true;
}

void MQTTClient::disconnect() {
  if (state_ == MQTTConnectionState::DISCONNECTED) {
    return;
  }
  
  if (uart_log_) {
    uart_log_->send_string("[MQTT] Disconnecting...\r\n");
  }
  
  // Send DISCONNECT packet
  uint8_t buffer[2];
  buffer[0] = (static_cast<uint8_t>(MQTTMessageType::DISCONNECT) << 4);
  buffer[1] = 0; // Remaining length = 0
  
  send_packet(buffer, 2);
  
  // Close socket
  socket_manager_->close_socket(socket_number_);
  
  state_ = MQTTConnectionState::DISCONNECTED;
}

bool MQTTClient::reconnect() {
  if (state_ == MQTTConnectionState::CONNECTED) {
    return true;
  }
  
  uint32_t now = millis();
  if (now - last_reconnect_attempt_ < (config_.reconnect_interval * 1000)) {
    return false; // Too soon to reconnect
  }
  
  last_reconnect_attempt_ = now;
  
  if (uart_log_) {
    uart_log_->send_string("[MQTT] Attempting reconnection...\r\n");
  }
  
  state_ = MQTTConnectionState::RECONNECTING;
  
  // Close existing connection if any
  socket_manager_->close_socket(socket_number_);
  
  #ifdef __AVR__
  _delay_ms(100);
  #endif
  
  // Try to connect again
  return connect(config_);
}

void MQTTClient::loop() {
  if (state_ == MQTTConnectionState::DISCONNECTED) {
    return;
  }
  
  if (state_ != MQTTConnectionState::CONNECTED) {
    // Try to reconnect
    reconnect();
    return;
  }
  
  // Process incoming packets
  process_incoming_packets();
  
  // Handle keepalive
  update_keepalive();
}

bool MQTTClient::publish(const char* topic, const uint8_t* payload, uint16_t length,
                         MQTTQoS qos, bool retain) {
  if (state_ != MQTTConnectionState::CONNECTED) {
    if (uart_log_) {
      uart_log_->send_string("[MQTT] Cannot publish: not connected\r\n");
    }
    return false;
  }
  
  uint16_t topic_len = strlen(topic);
  if (topic_len > 64 || length > MQTT_MAX_PAYLOAD_SIZE) {
    return false;
  }
  
  uint16_t idx = 0;
  
  // Fixed header
  uint8_t flags = 0;
  if (retain) flags |= 0x01;
  flags |= (static_cast<uint8_t>(qos) << 1);
  
  tx_buffer_[idx++] = (static_cast<uint8_t>(MQTTMessageType::PUBLISH) << 4) | flags;
  
  // Variable header + payload length
  uint16_t remaining_length = 2 + topic_len + length;
  if (qos != MQTTQoS::QoS0) {
    remaining_length += 2; // Packet identifier
  }
  
  idx += encode_remaining_length(&tx_buffer_[idx], remaining_length);
  
  // Topic name
  tx_buffer_[idx++] = (topic_len >> 8) & 0xFF;
  tx_buffer_[idx++] = topic_len & 0xFF;
  memcpy(&tx_buffer_[idx], topic, topic_len);
  idx += topic_len;
  
  // Packet identifier (for QoS > 0)
  if (qos != MQTTQoS::QoS0) {
    tx_buffer_[idx++] = (packet_id_ >> 8) & 0xFF;
    tx_buffer_[idx++] = packet_id_ & 0xFF;
    packet_id_++;
  }
  
  // Payload
  memcpy(&tx_buffer_[idx], payload, length);
  idx += length;
  
  bool result = send_packet(tx_buffer_, idx);
  
  if (result && uart_log_) {
    uart_log_->send_string("[MQTT] Published to topic: ");
    uart_log_->send_string(topic);
    uart_log_->send_string("\r\n");
  }
  
  return result;
}

bool MQTTClient::publish(const char* topic, const char* payload,
                         MQTTQoS qos, bool retain) {
  return publish(topic, reinterpret_cast<const uint8_t*>(payload), 
                 strlen(payload), qos, retain);
}

bool MQTTClient::subscribe(const char* topic, MQTTQoS qos, MQTTMessageCallback callback) {
  if (state_ != MQTTConnectionState::CONNECTED) {
    return false;
  }
  
  if (subscription_count_ >= MQTT_MAX_TOPICS) {
    return false; // No more subscription slots
  }
  
  uint16_t topic_len = strlen(topic);
  if (topic_len > 47) {
    return false; // Topic too long
  }
  
  // Add to subscription list
  uint8_t sub_idx = subscription_count_++;
  strncpy(subscriptions_[sub_idx].topic, topic, 47);
  subscriptions_[sub_idx].topic[47] = '\0';
  subscriptions_[sub_idx].qos = qos;
  subscriptions_[sub_idx].callback = callback;
  subscriptions_[sub_idx].active = true;
  
  // Build SUBSCRIBE packet
  uint16_t idx = 0;
  
  tx_buffer_[idx++] = (static_cast<uint8_t>(MQTTMessageType::SUBSCRIBE) << 4) | 0x02; // QoS 1 required for SUBSCRIBE
  
  uint16_t remaining_length = 2 + 2 + topic_len + 1; // Packet ID + topic length + topic + QoS
  idx += encode_remaining_length(&tx_buffer_[idx], remaining_length);
  
  // Packet identifier
  tx_buffer_[idx++] = (packet_id_ >> 8) & 0xFF;
  tx_buffer_[idx++] = packet_id_ & 0xFF;
  packet_id_++;
  
  // Topic filter
  tx_buffer_[idx++] = (topic_len >> 8) & 0xFF;
  tx_buffer_[idx++] = topic_len & 0xFF;
  memcpy(&tx_buffer_[idx], topic, topic_len);
  idx += topic_len;
  
  // QoS
  tx_buffer_[idx++] = static_cast<uint8_t>(qos);
  
  bool result = send_packet(tx_buffer_, idx);
  
  if (result && uart_log_) {
    uart_log_->send_string("[MQTT] Subscribed to topic: ");
    uart_log_->send_string(topic);
    uart_log_->send_string("\r\n");
  }
  
  return result;
}

bool MQTTClient::unsubscribe(const char* topic) {
  // Find and remove from subscription list
  for (uint8_t i = 0; i < subscription_count_; i++) {
    if (subscriptions_[i].active && strcmp(subscriptions_[i].topic, topic) == 0) {
      subscriptions_[i].active = false;
      return true;
    }
  }
  return false;
}

// Private methods

bool MQTTClient::connect_to_broker() {
  // Create socket status
  Ethernet::W5500SocketStatus socket_status = {
    .socket_number = static_cast<Ethernet::W5500SocketNumber>(socket_number_),
    .protocol_type = Ethernet::W5500SocketProtocol::kTCP,
    .port = 0, // Client port (random)
    .current_state = Ethernet::W5500SocketState::kSOCK_CLOSED,
    .socket_flags = Ethernet::W5500SocketFlag::kNone
  };
  
  // Open socket
  if (socket_manager_->open_socket(socket_status) != SOCK_OK) {
    return false;
  }
  
  // TODO: DNS resolution for hostname - for now assume IP address in hostname field
  // Parse IP address from config_.broker_hostname
  uint8_t ip[4] = {0};
  // Simple IP parsing (assumes format xxx.xxx.xxx.xxx)
  // For production, use proper DNS resolution
  
  // Connect to broker
  if (socket_manager_->connect_socket(socket_number_, ip, config_.broker_port) != SOCK_OK) {
    return false;
  }
  
  // Wait for connection
  for (uint8_t i = 0; i < 50; i++) {
    socket_status = socket_manager_->get_socket_status(socket_number_);
    if (socket_status.current_state == Ethernet::W5500SocketState::kSOCK_ESTABLISHED) {
      return true;
    }
    #ifdef __AVR__
    _delay_ms(100);
    #endif
  }
  
  return false;
}

bool MQTTClient::send_connect_packet() {
  uint16_t idx = 0;
  
  // Fixed header
  tx_buffer_[idx++] = (static_cast<uint8_t>(MQTTMessageType::CONNECT) << 4);
  
  // Calculate remaining length
  uint16_t remaining_length = 10; // Variable header
  remaining_length += 2 + strlen(config_.client_id); // Client ID
  
  bool has_username = (config_.username[0] != '\0');
  bool has_password = (config_.password[0] != '\0');
  
  if (has_username) {
    remaining_length += 2 + strlen(config_.username);
  }
  if (has_password) {
    remaining_length += 2 + strlen(config_.password);
  }
  
  idx += encode_remaining_length(&tx_buffer_[idx], remaining_length);
  
  // Protocol name
  tx_buffer_[idx++] = 0x00;
  tx_buffer_[idx++] = 0x04;
  tx_buffer_[idx++] = 'M';
  tx_buffer_[idx++] = 'Q';
  tx_buffer_[idx++] = 'T';
  tx_buffer_[idx++] = 'T';
  
  // Protocol level
  tx_buffer_[idx++] = MQTT_PROTOCOL_LEVEL;
  
  // Connect flags
  uint8_t flags = 0x02; // Clean session
  if (has_username) flags |= 0x80;
  if (has_password) flags |= 0x40;
  tx_buffer_[idx++] = flags;
  
  // Keepalive
  tx_buffer_[idx++] = (config_.keepalive >> 8) & 0xFF;
  tx_buffer_[idx++] = config_.keepalive & 0xFF;
  
  // Client ID
  idx += encode_string(&tx_buffer_[idx], config_.client_id);
  
  // Username
  if (has_username) {
    idx += encode_string(&tx_buffer_[idx], config_.username);
  }
  
  // Password
  if (has_password) {
    idx += encode_string(&tx_buffer_[idx], config_.password);
  }
  
  return send_packet(tx_buffer_, idx);
}

bool MQTTClient::wait_for_connack() {
  // Wait for CONNACK with timeout
  for (uint8_t i = 0; i < 30; i++) {
    uint16_t length = 0;
    if (receive_packet(rx_buffer_, &length)) {
      if (length >= 2) {
        uint8_t msg_type = (rx_buffer_[0] >> 4);
        if (msg_type == static_cast<uint8_t>(MQTTMessageType::CONNACK)) {
          // Check return code (byte at index 3)
          if (length >= 4 && rx_buffer_[3] == 0) {
            return true;
          }
        }
      }
    }
    #ifdef __AVR__
    _delay_ms(100);
    #endif
  }
  return false;
}

void MQTTClient::handle_disconnect() {
  state_ = MQTTConnectionState::DISCONNECTED;
  socket_manager_->close_socket(socket_number_);
}

bool MQTTClient::send_packet(const uint8_t* buffer, uint16_t length) {
  int32_t sent = socket_manager_->send_socket(socket_number_, 
                                              const_cast<uint8_t*>(buffer), 
                                              length);
  last_activity_ = millis();
  return (sent == length);
}

bool MQTTClient::receive_packet(uint8_t* buffer, uint16_t* length) {
  int32_t received = socket_manager_->recv_socket(socket_number_, buffer, 
                                                  MQTT_MAX_PACKET_SIZE);
  if (received > 0) {
    *length = static_cast<uint16_t>(received);
    last_activity_ = millis();
    return true;
  }
  return false;
}

void MQTTClient::process_incoming_packets() {
  uint16_t length = 0;
  if (receive_packet(rx_buffer_, &length)) {
    if (length < 2) return;
    
    uint8_t msg_type = (rx_buffer_[0] >> 4);
    
    switch (static_cast<MQTTMessageType>(msg_type)) {
      case MQTTMessageType::PUBLISH:
        handle_publish(rx_buffer_, length);
        break;
      case MQTTMessageType::CONNACK:
        handle_connack(rx_buffer_, length);
        break;
      case MQTTMessageType::SUBACK:
        handle_suback(rx_buffer_, length);
        break;
      case MQTTMessageType::PINGRESP:
        handle_pingresp();
        break;
      default:
        break;
    }
  }
}

void MQTTClient::handle_publish(const uint8_t* buffer, uint16_t length) {
  if (length < 4) return;
  
  // Extract topic
  char topic[48];
  uint16_t idx = 1;
  uint16_t remaining_len = 0;
  idx += decode_remaining_length(&buffer[idx], &remaining_len);
  
  uint16_t topic_len = (buffer[idx] << 8) | buffer[idx + 1];
  idx += 2;
  
  if (topic_len > 47) topic_len = 47;
  memcpy(topic, &buffer[idx], topic_len);
  topic[topic_len] = '\0';
  idx += topic_len;
  
  // Extract payload
  uint16_t payload_len = length - idx;
  const uint8_t* payload = &buffer[idx];
  
  if (uart_log_) {
    uart_log_->send_string("[MQTT] Received message on topic: ");
    uart_log_->send_string(topic);
    uart_log_->send_string("\r\n");
  }
  
  // Find matching subscription and call callback
  for (uint8_t i = 0; i < subscription_count_; i++) {
    if (subscriptions_[i].active && strcmp(subscriptions_[i].topic, topic) == 0) {
      if (subscriptions_[i].callback) {
        subscriptions_[i].callback(topic, payload, payload_len);
      }
      break;
    }
  }
}

void MQTTClient::handle_connack(const uint8_t* buffer, uint16_t length) {
  // CONNACK already handled in wait_for_connack
}

void MQTTClient::handle_suback(const uint8_t* buffer, uint16_t length) {
  // Subscription acknowledged
  if (uart_log_) {
    uart_log_->send_string("[MQTT] SUBACK received\r\n");
  }
}

void MQTTClient::handle_pingresp() {
  pending_ping_ = false;
  if (uart_log_) {
    uart_log_->send_string("[MQTT] PINGRESP received\r\n");
  }
}

uint16_t MQTTClient::encode_remaining_length(uint8_t* buffer, uint16_t length) {
  uint16_t idx = 0;
  do {
    uint8_t encoded_byte = length % 128;
    length /= 128;
    if (length > 0) {
      encoded_byte |= 0x80;
    }
    buffer[idx++] = encoded_byte;
  } while (length > 0);
  return idx;
}

uint16_t MQTTClient::decode_remaining_length(const uint8_t* buffer, uint16_t* value) {
  uint16_t multiplier = 1;
  uint16_t idx = 0;
  *value = 0;
  
  uint8_t encoded_byte;
  do {
    encoded_byte = buffer[idx++];
    *value += (encoded_byte & 0x7F) * multiplier;
    multiplier *= 128;
  } while ((encoded_byte & 0x80) != 0);
  
  return idx;
}

uint16_t MQTTClient::encode_string(uint8_t* buffer, const char* str) {
  uint16_t len = strlen(str);
  buffer[0] = (len >> 8) & 0xFF;
  buffer[1] = len & 0xFF;
  memcpy(&buffer[2], str, len);
  return len + 2;
}

uint16_t MQTTClient::decode_string(const uint8_t* buffer, char* str, uint16_t max_len) {
  uint16_t len = (buffer[0] << 8) | buffer[1];
  if (len > max_len - 1) {
    len = max_len - 1;
  }
  memcpy(str, &buffer[2], len);
  str[len] = '\0';
  return len + 2;
}

void MQTTClient::send_ping() {
  tx_buffer_[0] = (static_cast<uint8_t>(MQTTMessageType::PINGREQ) << 4);
  tx_buffer_[1] = 0; // Remaining length = 0
  
  if (send_packet(tx_buffer_, 2)) {
    pending_ping_ = true;
    last_ping_ = millis();
    if (uart_log_) {
      uart_log_->send_string("[MQTT] PINGREQ sent\r\n");
    }
  }
}

void MQTTClient::update_keepalive() {
  uint32_t now = millis();
  
  // Send ping if no activity for 3/4 of keepalive period
  if (now - last_activity_ > (config_.keepalive * 750)) {
    if (!pending_ping_) {
      send_ping();
    } else if (now - last_ping_ > (config_.keepalive * 1000)) {
      // Ping timeout - disconnect
      if (uart_log_) {
        uart_log_->send_string("[MQTT] Ping timeout - disconnecting\r\n");
      }
      handle_disconnect();
    }
  }
}

} // namespace MQTT
