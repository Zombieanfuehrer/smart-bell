#include "Network/MQTTClient.h"
#include <string.h>
#include "SetupWDT.h"
#include "System/TimerService.h"

// Include ioLibrary MQTT headers
extern "C" {
#include "MQTTClient.h"
#include "mqtt_interface.h"
#include "socket.h"
}

namespace Network {

// Static instance for callbacks
MQTTClient* MQTTClient::instance_ = nullptr;

MQTTClient::MQTTClient(serial::Interface* uart)
    : uart_(uart),
      status_(MQTTStatus::kDisconnected),
      has_credentials_(false),
      reconnect_interval_sec_(kDefaultReconnectInterval),
      last_disconnect_time_(0),
      reconnect_pending_(false),
      default_callback_(nullptr),
      mqtt_client_(nullptr),
      network_(nullptr),
      initialized_(false) {
  memset(send_buffer_, 0, kSendBufferSize);
  memset(recv_buffer_, 0, kRecvBufferSize);
  memset(&last_config_, 0, sizeof(MQTTConfig));
  memset(&last_credentials_, 0, sizeof(MQTTCredentials));
  instance_ = this;
}

MQTTClient::~MQTTClient() {
  if (status_ == MQTTStatus::kConnected) {
    disconnect();
  }

  // Free allocated structures
  if (mqtt_client_ != nullptr) {
    delete static_cast<::MQTTClient*>(mqtt_client_);
    mqtt_client_ = nullptr;
  }
  if (network_ != nullptr) {
    delete static_cast<::Network*>(network_);
    network_ = nullptr;
  }

  if (instance_ == this) {
    instance_ = nullptr;
  }
}

void MQTTClient::init() {
  log("[MQTT] Initializing...\r\n");

  // Allocate MQTT client and network structures
  if (mqtt_client_ == nullptr) {
    mqtt_client_ = new ::MQTTClient();
    memset(mqtt_client_, 0, sizeof(::MQTTClient));
  }
  if (network_ == nullptr) {
    network_ = new ::Network();
    memset(network_, 0, sizeof(::Network));
  }

  initialized_ = true;
  status_ = MQTTStatus::kDisconnected;

  log("[MQTT] Ready\r\n");
}

bool MQTTClient::connect(const MQTTConfig& config, const MQTTCredentials* credentials) {
  if (!initialized_) {
    log("[MQTT] Error: Not initialized\r\n");
    return false;
  }

  log("[MQTT] Connecting to broker...\r\n");
  log_ip("[MQTT] Broker: ", config.broker_ip);

  status_ = MQTTStatus::kConnecting;
  reconnect_pending_ = false;

  // Store config for reconnection
  memcpy(&last_config_, &config, sizeof(MQTTConfig));
  if (credentials != nullptr) {
    last_credentials_.username = credentials->username;
    last_credentials_.password = credentials->password;
    has_credentials_ = true;
  } else {
    has_credentials_ = false;
  }

  // Pause WDT during connection
  configure_wdt::pause();

  // Initialize network
  ::Network* net = static_cast<::Network*>(network_);
  NewNetwork(net, kMQTTSocket);

  // Connect network to broker
  int rc = ConnectNetwork(net, const_cast<uint8_t*>(config.broker_ip), config.broker_port);
  if (rc != 0) {
    log("[MQTT] Network connection failed\r\n");
    configure_wdt::resume();
    status_ = MQTTStatus::kError;
    last_disconnect_time_ = System::TimerService::seconds();
    reconnect_pending_ = true;
    return false;
  }

  // Initialize MQTT client
  ::MQTTClient* client = static_cast<::MQTTClient*>(mqtt_client_);
  MQTTClientInit(client, net, kCommandTimeout, send_buffer_, kSendBufferSize, recv_buffer_,
                 kRecvBufferSize);

  // Set up connect data
  MQTTPacket_connectData connect_data = MQTTPacket_connectData_initializer;
  connect_data.MQTTVersion = 4;  // MQTT 3.1.1
  connect_data.clientID.cstring = const_cast<char*>(config.client_id);
  connect_data.keepAliveInterval = config.keepalive_sec;
  connect_data.cleansession = config.clean_session ? 1 : 0;

  // Set authentication if provided
  if (credentials != nullptr && credentials->username != nullptr) {
    connect_data.username.cstring = const_cast<char*>(credentials->username);
    if (credentials->password != nullptr) {
      connect_data.password.cstring = const_cast<char*>(credentials->password);
    }
    log("[MQTT] Using authentication\r\n");
  }

  // Connect to broker
  rc = MQTTConnect(client, &connect_data);

  configure_wdt::resume();
  configure_wdt::reset();

  if (rc != 0) {
    log("[MQTT] MQTT connection failed\r\n");
    status_ = MQTTStatus::kError;
    last_disconnect_time_ = System::TimerService::seconds();
    reconnect_pending_ = true;
    return false;
  }

  log("[MQTT] Connected successfully\r\n");
  status_ = MQTTStatus::kConnected;
  return true;
}

void MQTTClient::disconnect() {
  if (status_ != MQTTStatus::kConnected) {
    return;
  }

  log("[MQTT] Disconnecting...\r\n");

  ::MQTTClient* client = static_cast<::MQTTClient*>(mqtt_client_);
  MQTTDisconnect(client);

  // Close socket
  close(kMQTTSocket);

  status_ = MQTTStatus::kDisconnected;
  last_disconnect_time_ = System::TimerService::seconds();

  log("[MQTT] Disconnected\r\n");
}

bool MQTTClient::is_connected() const {
  if (!initialized_ || mqtt_client_ == nullptr) {
    return false;
  }
  ::MQTTClient* client = static_cast<::MQTTClient*>(mqtt_client_);
  return client->isconnected != 0;
}

MQTTStatus MQTTClient::get_status() const { return status_; }

bool MQTTClient::publish(const char* topic, const uint8_t* payload, uint16_t length, MQTTQoS qos,
                         bool retained) {
  if (!is_connected()) {
    log("[MQTT] Cannot publish: not connected\r\n");
    return false;
  }

  log("[MQTT] Publishing to: ");
  log(topic);
  log("\r\n");

  configure_wdt::reset();

  MQTTMessage message;
  message.qos = static_cast<enum QoS>(static_cast<uint8_t>(qos));
  message.retained = retained ? 1 : 0;
  message.dup = 0;
  message.payload = const_cast<uint8_t*>(payload);
  message.payloadlen = length;

  ::MQTTClient* client = static_cast<::MQTTClient*>(mqtt_client_);
  int rc = MQTTPublish(client, topic, &message);

  configure_wdt::reset();

  if (rc != 0) {
    log("[MQTT] Publish failed\r\n");
    // Check if connection was lost
    if (!is_connected()) {
      status_ = MQTTStatus::kDisconnected;
      last_disconnect_time_ = System::TimerService::seconds();
      reconnect_pending_ = true;
    }
    return false;
  }

  log("[MQTT] Published successfully\r\n");
  return true;
}

bool MQTTClient::publish_string(const char* topic, const char* message, MQTTQoS qos) {
  return publish(topic, reinterpret_cast<const uint8_t*>(message),
                 static_cast<uint16_t>(strlen(message)), qos, false);
}

bool MQTTClient::subscribe(const char* topic, MQTTMessageCallback callback, MQTTQoS qos) {
  if (!is_connected()) {
    log("[MQTT] Cannot subscribe: not connected\r\n");
    return false;
  }

  log("[MQTT] Subscribing to: ");
  log(topic);
  log("\r\n");

  // Store callback for this topic
  default_callback_ = callback;

  configure_wdt::reset();

  ::MQTTClient* client = static_cast<::MQTTClient*>(mqtt_client_);
  int rc = MQTTSubscribe(client, topic, static_cast<enum QoS>(static_cast<uint8_t>(qos)),
                         internal_message_handler);

  configure_wdt::reset();

  if (rc != 0) {
    log("[MQTT] Subscribe failed\r\n");
    return false;
  }

  log("[MQTT] Subscribed successfully\r\n");
  return true;
}

bool MQTTClient::unsubscribe(const char* topic) {
  if (!is_connected()) {
    return false;
  }

  log("[MQTT] Unsubscribing from: ");
  log(topic);
  log("\r\n");

  ::MQTTClient* client = static_cast<::MQTTClient*>(mqtt_client_);
  int rc = MQTTUnsubscribe(client, topic);

  if (rc != 0) {
    log("[MQTT] Unsubscribe failed\r\n");
    return false;
  }

  return true;
}

bool MQTTClient::yield(uint32_t timeout_ms) {
  if (!is_connected()) {
    return false;
  }

  configure_wdt::reset();

  ::MQTTClient* client = static_cast<::MQTTClient*>(mqtt_client_);
  int rc = MQTTYield(client, static_cast<int>(timeout_ms));

  configure_wdt::reset();

  if (rc != 0) {
    log("[MQTT] Yield failed, connection may be lost\r\n");
    if (!is_connected()) {
      status_ = MQTTStatus::kDisconnected;
      last_disconnect_time_ = System::TimerService::seconds();
      reconnect_pending_ = true;
    }
    return false;
  }

  return true;
}

void MQTTClient::set_reconnect_interval(uint32_t seconds) { reconnect_interval_sec_ = seconds; }

uint32_t MQTTClient::get_reconnect_interval() const { return reconnect_interval_sec_; }

bool MQTTClient::is_reconnect_pending() const { return reconnect_pending_; }

bool MQTTClient::try_reconnect() {
  if (!reconnect_pending_) {
    return false;
  }

  uint32_t now = System::TimerService::seconds();
  if ((now - last_disconnect_time_) < reconnect_interval_sec_) {
    return false;  // Not time yet
  }

  log("[MQTT] Attempting reconnection...\r\n");

  // Try to reconnect with stored config
  if (has_credentials_) {
    return connect(last_config_, &last_credentials_);
  } else {
    return connect(last_config_, nullptr);
  }
}

void MQTTClient::set_default_message_handler(MQTTMessageCallback callback) {
  default_callback_ = callback;
}

void MQTTClient::internal_message_handler(void* md) {
  if (instance_ == nullptr || instance_->default_callback_ == nullptr) {
    return;
  }

  MessageData* message_data = static_cast<MessageData*>(md);

  MQTTMessageData data;
  data.topic = message_data->topicName->lenstring.data;
  data.topic_length = message_data->topicName->lenstring.len;
  data.payload = static_cast<const uint8_t*>(message_data->message->payload);
  data.payload_length = static_cast<uint16_t>(message_data->message->payloadlen);
  data.qos = static_cast<MQTTQoS>(message_data->message->qos);
  data.retained = message_data->message->retained != 0;

  instance_->log("[MQTT] Message received on topic\r\n");

  instance_->default_callback_(&data);
}

void MQTTClient::log(const char* message) {
  if (uart_ != nullptr) {
    uart_->send_string(message);
  }
}

void MQTTClient::log_ip(const char* label, const uint8_t* ip) {
  if (uart_ == nullptr)
    return;

  uart_->send_string(label);

  char buffer[4];
  for (int i = 0; i < 4; i++) {
    uint8_t val = ip[i];
    int pos = 0;
    if (val >= 100) {
      buffer[pos++] = '0' + (val / 100);
      val %= 100;
      buffer[pos++] = '0' + (val / 10);
      val %= 10;
    } else if (val >= 10) {
      buffer[pos++] = '0' + (val / 10);
      val %= 10;
    }
    buffer[pos++] = '0' + val;
    buffer[pos] = '\0';

    uart_->send_string(buffer);
    if (i < 3) {
      uart_->send('.');
    }
  }
  uart_->send_string("\r\n");
}

}  // namespace Network
