#include "MQTT/MinimalMQTT.h"

// Platform-specific includes
#ifdef __AVR__
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#else
// Test/Linux build
#ifndef PROGMEM
#define PROGMEM
#endif
#ifndef PSTR
#define PSTR(s) (s)
#endif
#endif

#include <string.h>
#include "System/TimerService.h"

#ifdef __AVR__
// W5500 socket API
extern "C" {
#include "W5500/w5500.h"  // for getPHYCFGR() / PHYCFGR_LNK_ON
#include "socket.h"
}
#else
// Test build - mock socket API (provided by test fixture)
#endif

namespace MQTT {

MinimalMQTT::MinimalMQTT(serial::UART* uart)
    : uart_(uart), state_(State::DISCONNECTED), last_activity_(0), last_ping_(0), packet_id_(1) {
  memset(&config_, 0, sizeof(Config));
  memset(send_buffer_, 0, kSendBufferSize);
  memset(recv_buffer_, 0, kRecvBufferSize);

  // Initialize subscriptions
  for (uint8_t i = 0; i < kMaxSubscriptions; i++) {
    subscriptions_[i].active = false;
    subscriptions_[i].callback = nullptr;
  }
}

bool MinimalMQTT::connect(const Config& config) {
  log("[MQTT] Connecting...\r\n");

  // Remove all existing subscriptions on new connect attempt
  for (uint8_t i = 0; i < kMaxSubscriptions; i++) {
    subscriptions_[i].active = false;
    subscriptions_[i].callback = nullptr;
  }

  // Store config
  memcpy(&config_, &config, sizeof(Config));
  state_ = State::CONNECTING;

  // Connect TCP socket to broker
  if (!socket_connect()) {
    log("[MQTT] Socket connect failed\r\n");
    state_ = State::ERROR;
    return false;
  }

  // Send CONNECT packet
  if (!send_connect_packet()) {
    log("[MQTT] CONNECT failed\r\n");
    socket_disconnect();
    state_ = State::ERROR;
    return false;
  }

  // Wait for CONNACK
  if (!wait_for_connack()) {
    log("[MQTT] CONNACK failed\r\n");
    socket_disconnect();
    state_ = State::ERROR;
    return false;
  }

  log("[MQTT] Connected\r\n");
  state_ = State::CONNECTED;
  last_activity_ = System::TimerService::millis();
  last_ping_ = last_activity_;

  return true;
}

void MinimalMQTT::disconnect() {
  if (state_ == State::DISCONNECTED) {
    return;
  }

  // Send DISCONNECT packet
  send_buffer_[0] = static_cast<uint8_t>(MessageType::DISCONNECT);
  send_buffer_[1] = 0;  // Remaining length = 0
  socket_send(send_buffer_, 2);

  socket_disconnect();
  state_ = State::DISCONNECTED;

  // Remove all subscriptions
  for (uint8_t i = 0; i < kMaxSubscriptions; i++) {
    subscriptions_[i].active = false;
    subscriptions_[i].callback = nullptr;
  }

  log("[MQTT] Disconnected\r\n");
}

bool MinimalMQTT::publish(const char* topic, const uint8_t* payload, uint16_t length) {
  if (state_ != State::CONNECTED) {
    return false;
  }

  uint16_t pos = 0;

  // Fixed header: PUBLISH + QoS0
  send_buffer_[pos++] = static_cast<uint8_t>(MessageType::PUBLISH);

  // Calculate remaining length
  uint16_t topic_len = strlen(topic);
  uint16_t remaining = 2 + topic_len + length;  // topic_len(2) + topic + payload

  // Check buffer size
  if (remaining + 2 > kSendBufferSize) {
    log("[MQTT] Publish too large\r\n");
    return false;
  }

  // Encode remaining length (1-byte for small messages)
  if (remaining < 128) {
    send_buffer_[pos++] = remaining & 0x7F;
  } else {
    send_buffer_[pos++] = (remaining & 0x7F) | 0x80;
    send_buffer_[pos++] = (remaining >> 7) & 0x7F;
  }

  // Topic length
  send_buffer_[pos++] = (topic_len >> 8) & 0xFF;
  send_buffer_[pos++] = topic_len & 0xFF;

  // Topic
  memcpy(&send_buffer_[pos], topic, topic_len);
  pos += topic_len;

  // Payload
  memcpy(&send_buffer_[pos], payload, length);
  pos += length;

  // Send
  bool success = socket_send(send_buffer_, pos);
  if (success) {
    last_activity_ = System::TimerService::millis();
  }

  return success;
}

bool MinimalMQTT::publish_string(const char* topic, const char* message) {
  return publish(topic, reinterpret_cast<const uint8_t*>(message), strlen(message));
}

bool MinimalMQTT::subscribe(const char* topic, MessageCallback callback) {
  if (state_ != State::CONNECTED || callback == nullptr) {
    return false;
  }

  // Find free subscription slot
  uint8_t slot = kMaxSubscriptions;
  for (uint8_t i = 0; i < kMaxSubscriptions; i++) {
    if (!subscriptions_[i].active) {
      slot = i;
      break;
    }
  }

  if (slot >= kMaxSubscriptions) {
    log("[MQTT] No free subscription slots\r\n");
    return false;
  }

  uint16_t pos = 0;

  // Fixed header: SUBSCRIBE
  send_buffer_[pos++] = static_cast<uint8_t>(MessageType::SUBSCRIBE);

  // Calculate remaining length
  uint16_t topic_len = strlen(topic);
  uint16_t remaining = 2 + 2 + topic_len + 1;  // packet_id(2) + topic_len(2) + topic + qos(1)

  // Encode remaining length
  if (remaining < 128) {
    send_buffer_[pos++] = remaining & 0x7F;
  } else {
    send_buffer_[pos++] = (remaining & 0x7F) | 0x80;
    send_buffer_[pos++] = (remaining >> 7) & 0x7F;
  }

  // Packet ID
  send_buffer_[pos++] = (packet_id_ >> 8) & 0xFF;
  send_buffer_[pos++] = packet_id_ & 0xFF;
  packet_id_++;

  // Topic length
  send_buffer_[pos++] = (topic_len >> 8) & 0xFF;
  send_buffer_[pos++] = topic_len & 0xFF;

  // Topic
  memcpy(&send_buffer_[pos], topic, topic_len);
  pos += topic_len;

  // QoS = 0
  send_buffer_[pos++] = 0;

  // Send
  bool success = socket_send(send_buffer_, pos);
  if (success) {
    // Store subscription
    strncpy(subscriptions_[slot].topic, topic, kMaxTopicLength - 1);
    subscriptions_[slot].topic[kMaxTopicLength - 1] = '\0';
    subscriptions_[slot].callback = callback;
    subscriptions_[slot].active = true;

    last_activity_ = System::TimerService::millis();
    log("[MQTT] Subscribed\r\n");
  }

  return success;
}

void MinimalMQTT::loop() {
  if (state_ != State::CONNECTED) {
    return;
  }

  // Check socket status
  if (!socket_is_connected()) {
    log("[MQTT] Socket disconnected\r\n");
    state_ = State::ERROR;
    return;
  }

  // Process incoming messages
  process_incoming_packet();

  // Keepalive: send PINGREQ if idle
  uint32_t now = System::TimerService::millis();
  uint32_t idle_time = now - last_activity_;

  // Send ping at 75% of keepalive interval
  uint32_t ping_interval = (config_.keepalive * 1000UL * 3) / 4;

  if ((now - last_ping_) > ping_interval) {
    send_pingreq();
  }

  // Timeout if no activity for keepalive * 1.5
  if (idle_time > (config_.keepalive * 1500UL)) {
    log("[MQTT] Keepalive timeout\r\n");
    state_ = State::ERROR;
  }
}

// ==================== Private Methods ====================

bool MinimalMQTT::socket_connect() {
  // Close socket if open
  close(kMQTTSocketNumber);

#ifdef __AVR__
  // W5500 link negotiation can lag behind reset/init; wait up to ~5s.
  uint8_t phy = 0;
  bool link_up = false;
  for (uint8_t i = 0; i < 50; i++) {
    phy = getPHYCFGR();
    if (phy & PHYCFGR_LNK_ON) {
      link_up = true;
      break;
    }
    wdt_reset();
    for (uint16_t j = 0; j < 1000; j++) {
      asm volatile("nop");
    }
  }
  if (!link_up) {
    log("[MQTT] No link\r\n");
    return false;
  }
#endif

  // Open TCP socket
  int8_t result = socket(kMQTTSocketNumber, Sn_MR_TCP, 0, 0);
  if (result != kMQTTSocketNumber) {
    return false;
  }

  // Connect to broker
  result = ::connect(kMQTTSocketNumber, config_.broker_ip, config_.broker_port);
  if (result != SOCK_OK) {
    close(kMQTTSocketNumber);
    return false;
  }

  // Wait for connection (timeout 5s)
  // wdt_reset() prevents WDT from firing during this blocking wait
  for (uint8_t i = 0; i < 50; i++) {
#ifdef __AVR__
    wdt_reset();
#endif
    uint8_t status = getSn_SR(kMQTTSocketNumber);
    if (status == SOCK_ESTABLISHED) {
      return true;
    }
    if (status == SOCK_CLOSED || status == SOCK_CLOSE_WAIT) {
      return false;
    }
    // Wait 100ms
    for (uint16_t j = 0; j < 1000; j++) {
      asm volatile("nop");
    }
  }

  return false;
}

void MinimalMQTT::socket_disconnect() {
  ::disconnect(kMQTTSocketNumber);
  close(kMQTTSocketNumber);
}

bool MinimalMQTT::socket_send(const uint8_t* data, uint16_t length) {
  // W5500 send() requires non-const pointer (but doesn't modify data)
  int32_t sent = send(kMQTTSocketNumber, const_cast<uint8_t*>(data), length);
  return (sent == length);
}

int16_t MinimalMQTT::socket_recv(uint8_t* buffer, uint16_t max_length) {
  int32_t available = getSn_RX_RSR(kMQTTSocketNumber);
  if (available <= 0) {
    return 0;
  }

  uint16_t to_read = (available > max_length) ? max_length : available;
  int32_t received = recv(kMQTTSocketNumber, buffer, to_read);

  return (received > 0) ? received : 0;
}

bool MinimalMQTT::socket_is_connected() {
  uint8_t status = getSn_SR(kMQTTSocketNumber);
  return (status == SOCK_ESTABLISHED);
}

bool MinimalMQTT::send_connect_packet() {
  uint16_t pos = 0;

  // Fixed header
  send_buffer_[pos++] = static_cast<uint8_t>(MessageType::CONNECT);

  // Calculate remaining length
  uint16_t remaining = 10;  // Variable header
  remaining += 2 + strlen(config_.client_id);
  if (config_.use_auth) {
    remaining += 2 + strlen(config_.username);
    remaining += 2 + strlen(config_.password);
  }

  // Remaining length (assume < 128 bytes)
  send_buffer_[pos++] = remaining & 0x7F;

  // Protocol name: "MQTT"
  send_buffer_[pos++] = 0;
  send_buffer_[pos++] = 4;
  send_buffer_[pos++] = 'M';
  send_buffer_[pos++] = 'Q';
  send_buffer_[pos++] = 'T';
  send_buffer_[pos++] = 'T';

  // Protocol level: 4 (MQTT 3.1.1)
  send_buffer_[pos++] = kMQTTProtocolLevel;

  // Connect flags
  uint8_t flags = 0x02;  // Clean session
  if (config_.use_auth) {
    flags |= 0x80;  // Username flag
    flags |= 0x40;  // Password flag
  }
  send_buffer_[pos++] = flags;

  // Keepalive
  send_buffer_[pos++] = (config_.keepalive >> 8) & 0xFF;
  send_buffer_[pos++] = config_.keepalive & 0xFF;

  // Client ID
  uint16_t id_len = strlen(config_.client_id);
  send_buffer_[pos++] = (id_len >> 8) & 0xFF;
  send_buffer_[pos++] = id_len & 0xFF;
  memcpy(&send_buffer_[pos], config_.client_id, id_len);
  pos += id_len;

  // Username & Password
  if (config_.use_auth) {
    uint16_t user_len = strlen(config_.username);
    send_buffer_[pos++] = (user_len >> 8) & 0xFF;
    send_buffer_[pos++] = user_len & 0xFF;
    memcpy(&send_buffer_[pos], config_.username, user_len);
    pos += user_len;

    uint16_t pass_len = strlen(config_.password);
    send_buffer_[pos++] = (pass_len >> 8) & 0xFF;
    send_buffer_[pos++] = pass_len & 0xFF;
    memcpy(&send_buffer_[pos], config_.password, pass_len);
    pos += pass_len;
  }

  return socket_send(send_buffer_, pos);
}

bool MinimalMQTT::wait_for_connack() {
  // Wait up to 5 seconds for CONNACK
  uint32_t start = System::TimerService::millis();

  while ((System::TimerService::millis() - start) < 5000) {
    int16_t len = socket_recv(recv_buffer_, kRecvBufferSize);
    if (len >= 4) {
      // Check for CONNACK
      if (recv_buffer_[0] == static_cast<uint8_t>(MessageType::CONNACK) && recv_buffer_[1] == 2) {
        // Check return code
        uint8_t return_code = recv_buffer_[3];
        if (return_code == 0) {
          return true;  // Connection accepted
        }
        log("[MQTT] CONNACK refused\r\n");
        return false;
      }
    }
  }

  log("[MQTT] CONNACK timeout\r\n");
  return false;
}

void MinimalMQTT::send_pingreq() {
  send_buffer_[0] = static_cast<uint8_t>(MessageType::PINGREQ);
  send_buffer_[1] = 0;

  if (socket_send(send_buffer_, 2)) {
    last_ping_ = System::TimerService::millis();
  }
}

void MinimalMQTT::process_incoming_packet() {
  int16_t len = socket_recv(recv_buffer_, kRecvBufferSize);
  if (len <= 0) {
    return;
  }

  last_activity_ = System::TimerService::millis();

  uint8_t header_byte = recv_buffer_[0];
  uint8_t msg_type = header_byte & 0xF0;

  // WICHTIG: Holt die QoS-Bits aus dem Header-Byte für die spätere Abfrage (qos > 0)
  uint8_t qos = (header_byte & 0x06) >> 1;

  // Handle PINGRESP
  if (msg_type == static_cast<uint8_t>(MessageType::PINGRESP)) {
    return;
  }

  // Handle PUBLISH
  if (msg_type == static_cast<uint8_t>(MessageType::PUBLISH)) {
    uint16_t pos = 1;
    uint32_t multiplier = 1;
    uint32_t rem_len = 0;
    uint8_t encoded_byte;

    // Remaining Length decodieren (Variable Byte Integer)
    do {
      if (pos >= static_cast<uint16_t>(len))
        return;  // Out-of-Bounds Schutz
      encoded_byte = recv_buffer_[pos++];
      rem_len += (encoded_byte & 127) * multiplier;
      multiplier *= 128;
    } while ((encoded_byte & 128) != 0);

    if (pos + 2 > static_cast<uint16_t>(len))
      return;  // Out-of-Bounds Schutz

    // Topic Length decodieren
    uint16_t topic_len = (recv_buffer_[pos] << 8) | recv_buffer_[pos + 1];
    pos += 2;

    if (pos + topic_len > static_cast<uint16_t>(len))
      return;  // Out-of-Bounds Schutz

    // ======================================================================
    // DEINE ÄNDERUNG: Sicher in lokalen Buffer extrahieren
    // ======================================================================
    char topic_str[kMaxTopicLength];
    uint16_t copy_len = (topic_len < kMaxTopicLength - 1) ? topic_len : (kMaxTopicLength - 1);
    memcpy(topic_str, &recv_buffer_[pos], copy_len);
    topic_str[copy_len] = '\0';
    pos += topic_len;

    // ======================================================================
    // DEINE ÄNDERUNG: Packet ID überspringen (falls QoS > 0)
    // ======================================================================
    if (qos > 0) {
      if (pos + 2 > static_cast<uint16_t>(len))
        return;  // Out-of-Bounds Schutz
      pos += 2;
    }

    // Payload bestimmen
    const uint8_t* payload = &recv_buffer_[pos];
    uint16_t payload_len = static_cast<uint16_t>(len) - pos;

    // ======================================================================
    // DEINE ÄNDERUNG: Callbacks aufrufen
    // ======================================================================
    for (uint8_t i = 0; i < kMaxSubscriptions; i++) {
      if (subscriptions_[i].active && strcmp(subscriptions_[i].topic, topic_str) == 0 &&
          subscriptions_[i].callback != nullptr) {
        subscriptions_[i].callback(topic_str, payload, payload_len);
      }
    }

    // Die fehlerhafte Zeile "recv_buffer_[...] = original_byte;" wurde gelöscht,
    // da sie dank deiner "topic_str"-Änderung nicht mehr gebraucht wird!
  }
}

void MinimalMQTT::log(const char* message) {
  if (uart_ != nullptr) {
    uart_->send_string(message);
  }
}

}  // namespace MQTT
