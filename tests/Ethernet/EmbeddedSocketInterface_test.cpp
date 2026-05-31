#include "Ethernet/EmbeddedSocketInterface.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "MockWiznetAPI.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SetArgPointee;

namespace Ethernet {

class EmbeddedSocketInterfaceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mock_wiznet_api_ = new MockWiznetAPI();
    g_mock_wiznet_api = mock_wiznet_api_;

    // W5500Interface Mock (wir brauchen es nur als Dummy)
    w5500_interface_ = nullptr;

    // Socket-Manager erstellen
    socket_manager_ = std::make_unique<EmbeddedSocketW5500>(w5500_interface_);
  }

  void TearDown() override {
    socket_manager_.reset();
    delete mock_wiznet_api_;
    g_mock_wiznet_api = nullptr;
  }

  MockWiznetAPI* mock_wiznet_api_;
  const W5500Interface* w5500_interface_;
  std::unique_ptr<EmbeddedSocketW5500> socket_manager_;
};

// Test: Socket öffnen erfolgreich
TEST_F(EmbeddedSocketInterfaceTest, OpenSocketSuccess) {
  W5500SocketStatus socket_status = {.socket_number = W5500SocketNumber::kSocket0,
                                     .protocol_type = W5500SocketProtocol::kTCP,
                                     .port = 80,
                                     .current_state = W5500SocketState::kSOCK_CLOSED,
                                     .socket_flags = W5500SocketFlag::kNone};

  // Mock erwartete Aufrufe
  EXPECT_CALL(*mock_wiznet_api_, socket(0, 6, 80, 0)).WillOnce(Return(SOCK_OK));
  EXPECT_CALL(*mock_wiznet_api_, getSn_SR(0))
      .WillOnce(Return(static_cast<uint8_t>(W5500SocketState::kSOCK_INIT)));

  // Socket öffnen
  int8_t result = socket_manager_->open_socket(socket_status);

  // Verifizieren
  EXPECT_EQ(result, SOCK_OK);
  EXPECT_EQ(socket_status.current_state, W5500SocketState::kSOCK_INIT);
}

// Test: Socket öffnen schlägt fehl
TEST_F(EmbeddedSocketInterfaceTest, OpenSocketFailure) {
  W5500SocketStatus socket_status = {.socket_number = W5500SocketNumber::kSocket1,
                                     .protocol_type = W5500SocketProtocol::kUDP,
                                     .port = 5000,
                                     .current_state = W5500SocketState::kSOCK_CLOSED,
                                     .socket_flags = W5500SocketFlag::kMulticastEnable};

  // Mock erwartete Aufrufe
  EXPECT_CALL(*mock_wiznet_api_, socket(1, 17, 5000, _)).WillOnce(Return(SOCK_ERROR));

  // Socket öffnen
  int8_t result = socket_manager_->open_socket(socket_status);

  // Verifizieren
  EXPECT_EQ(result, SOCK_ERROR);
  EXPECT_EQ(socket_status.current_state, W5500SocketState::kSOCK_CLOSED);
}

// Test: Socket schließen
TEST_F(EmbeddedSocketInterfaceTest, CloseSocket) {
  // Mock erwartete Aufrufe
  EXPECT_CALL(*mock_wiznet_api_, close(0)).WillOnce(Return(SOCK_OK));

  // Socket schließen
  int8_t result = socket_manager_->close_socket(0);

  // Verifizieren
  EXPECT_EQ(result, SOCK_OK);

  // Status prüfen
  W5500SocketStatus status = socket_manager_->get_socket_status(0);
  EXPECT_EQ(status.current_state, W5500SocketState::kSOCK_CLOSED);
}

// Test: Socket auf Listen-Modus setzen
TEST_F(EmbeddedSocketInterfaceTest, ListenSocket) {
  // Mock erwartete Aufrufe
  EXPECT_CALL(*mock_wiznet_api_, listen(0)).WillOnce(Return(SOCK_OK));
  EXPECT_CALL(*mock_wiznet_api_, getSn_SR(0))
      .WillOnce(Return(static_cast<uint8_t>(W5500SocketState::kSOCK_LISTEN)));

  // Socket auf Listen setzen
  int8_t result = socket_manager_->listen_socket(0);

  // Verifizieren
  EXPECT_EQ(result, SOCK_OK);

  // Status prüfen
  W5500SocketStatus status = socket_manager_->get_socket_status(0);
  EXPECT_EQ(status.current_state, W5500SocketState::kSOCK_LISTEN);
}

// Test: Socket verbinden
TEST_F(EmbeddedSocketInterfaceTest, ConnectSocket) {
  uint8_t addr[4] = {192, 168, 1, 100};
  uint16_t port = 8080;

  // Mock erwartete Aufrufe
  EXPECT_CALL(*mock_wiznet_api_, connect(0, _, 8080)).WillOnce(Return(SOCK_OK));
  EXPECT_CALL(*mock_wiznet_api_, getSn_SR(0))
      .WillOnce(Return(static_cast<uint8_t>(W5500SocketState::kSOCK_ESTABLISHED)));

  // Socket verbinden
  int8_t result = socket_manager_->connect_socket(0, addr, port);

  // Verifizieren
  EXPECT_EQ(result, SOCK_OK);

  // Status prüfen
  W5500SocketStatus status = socket_manager_->get_socket_status(0);
  EXPECT_EQ(status.current_state, W5500SocketState::kSOCK_ESTABLISHED);
}

// Test: Socket trennen
TEST_F(EmbeddedSocketInterfaceTest, DisconnectSocket) {
  // Mock erwartete Aufrufe
  EXPECT_CALL(*mock_wiznet_api_, disconnect(0)).WillOnce(Return(SOCK_OK));
  EXPECT_CALL(*mock_wiznet_api_, getSn_SR(0))
      .WillOnce(Return(static_cast<uint8_t>(W5500SocketState::kSOCK_CLOSED)));

  // Socket trennen
  int8_t result = socket_manager_->disconnect_socket(0);

  // Verifizieren
  EXPECT_EQ(result, SOCK_OK);

  // Status prüfen
  W5500SocketStatus status = socket_manager_->get_socket_status(0);
  EXPECT_EQ(status.current_state, W5500SocketState::kSOCK_CLOSED);
}

// Test: Daten senden
TEST_F(EmbeddedSocketInterfaceTest, SendData) {
  uint8_t buffer[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  uint16_t len = 10;

  // Mock erwartete Aufrufe
  EXPECT_CALL(*mock_wiznet_api_, send(0, _, len)).WillOnce(Return(len));

  // Daten senden
  int32_t result = socket_manager_->send_socket(0, buffer, len);

  // Verifizieren
  EXPECT_EQ(result, len);
}

// Test: Daten empfangen
TEST_F(EmbeddedSocketInterfaceTest, ReceiveData) {
  uint8_t buffer[10];
  uint16_t len = 10;

  // Mock erwartete Aufrufe
  EXPECT_CALL(*mock_wiznet_api_, recv(0, _, len))
      .WillOnce(DoAll(Invoke([](uint8_t, uint8_t* buf, uint16_t l) {
                        for (int i = 0; i < l; i++)
                          buf[i] = i;
                      }),
                      Return(len)));

  // Daten empfangen
  int32_t result = socket_manager_->recv_socket(0, buffer, len);

  // Verifizieren
  EXPECT_EQ(result, len);
  for (int i = 0; i < len; i++) {
    EXPECT_EQ(buffer[i], i);
  }
}

// Test: Socket-Status für ungültige Socket-Nummer
TEST_F(EmbeddedSocketInterfaceTest, GetInvalidSocketStatus) {
  // Status für ungültige Socket-Nummer abfragen
  W5500SocketStatus status = socket_manager_->get_socket_status(8);

  // Verifizieren - sollte einen leeren/ungültigen Status zurückgeben
  EXPECT_EQ(status.socket_number, W5500SocketNumber::kSocket0);
  EXPECT_EQ(status.port, 0);
}

// Test: Socket-Status nach mehreren Operationen
TEST_F(EmbeddedSocketInterfaceTest, SocketStatusAfterMultipleOperations) {
  W5500SocketStatus socket_status = {.socket_number = W5500SocketNumber::kSocket2,
                                     .protocol_type = W5500SocketProtocol::kTCP,
                                     .port = 443,
                                     .current_state = W5500SocketState::kSOCK_CLOSED,
                                     .socket_flags = W5500SocketFlag::kTCPNoDelay};

  // Socket öffnen
  EXPECT_CALL(*mock_wiznet_api_, socket(2, 6, 443, _)).WillOnce(Return(SOCK_OK));
  EXPECT_CALL(*mock_wiznet_api_, getSn_SR(2))
      .WillOnce(Return(static_cast<uint8_t>(W5500SocketState::kSOCK_INIT)))
      .WillOnce(Return(static_cast<uint8_t>(W5500SocketState::kSOCK_LISTEN)));

  socket_manager_->open_socket(socket_status);
  EXPECT_EQ(socket_status.current_state, W5500SocketState::kSOCK_INIT);

  // Auf Listen setzen
  EXPECT_CALL(*mock_wiznet_api_, listen(2)).WillOnce(Return(SOCK_OK));

  socket_manager_->listen_socket(2);

  // Status abrufen
  W5500SocketStatus current_status = socket_manager_->get_socket_status(2);
  EXPECT_EQ(current_status.current_state, W5500SocketState::kSOCK_LISTEN);
  EXPECT_EQ(current_status.port, 443);
  EXPECT_EQ(current_status.protocol_type, W5500SocketProtocol::kTCP);
}

// Test: Mehrere Sockets parallel
TEST_F(EmbeddedSocketInterfaceTest, MultipleSocketsParallel) {
  // Socket 0 öffnen
  W5500SocketStatus socket0 = {.socket_number = W5500SocketNumber::kSocket0,
                               .protocol_type = W5500SocketProtocol::kTCP,
                               .port = 80,
                               .current_state = W5500SocketState::kSOCK_CLOSED,
                               .socket_flags = W5500SocketFlag::kNone};

  // Socket 1 öffnen
  W5500SocketStatus socket1 = {.socket_number = W5500SocketNumber::kSocket1,
                               .protocol_type = W5500SocketProtocol::kUDP,
                               .port = 5000,
                               .current_state = W5500SocketState::kSOCK_CLOSED,
                               .socket_flags = W5500SocketFlag::kMulticastEnable};

  EXPECT_CALL(*mock_wiznet_api_, socket(0, 6, 80, 0)).WillOnce(Return(SOCK_OK));
  EXPECT_CALL(*mock_wiznet_api_, getSn_SR(0))
      .WillOnce(Return(static_cast<uint8_t>(W5500SocketState::kSOCK_INIT)));

  EXPECT_CALL(*mock_wiznet_api_, socket(1, 17, 5000, _)).WillOnce(Return(SOCK_OK));
  EXPECT_CALL(*mock_wiznet_api_, getSn_SR(1))
      .WillOnce(Return(static_cast<uint8_t>(W5500SocketState::kSOCK_UDP)));

  socket_manager_->open_socket(socket0);
  socket_manager_->open_socket(socket1);

  // Status beider Sockets prüfen
  W5500SocketStatus status0 = socket_manager_->get_socket_status(0);
  W5500SocketStatus status1 = socket_manager_->get_socket_status(1);

  EXPECT_EQ(status0.current_state, W5500SocketState::kSOCK_INIT);
  EXPECT_EQ(status0.port, 80);

  EXPECT_EQ(status1.current_state, W5500SocketState::kSOCK_UDP);
  EXPECT_EQ(status1.port, 5000);
}

}  // namespace Ethernet
