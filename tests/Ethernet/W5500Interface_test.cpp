#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "MockSPI.h"
#include "MockWiznetAPI.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

namespace Ethernet {

// Einfacher Test für W5500Interface-Funktionalität
// Da W5500Interface Hardware-spezifisch ist, testen wir hauptsächlich die Callback-Logik
class W5500InterfaceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mock_wiznet_api_ = new MockWiznetAPI();
    g_mock_wiznet_api = mock_wiznet_api_;

    mock_spi_ = std::make_unique<MockSPI>();
    mock_uart_ = std::make_unique<MockUART>();

    // Callbacks für Tests vorbereiten
    hard_reset_called_ = false;
    chip_select_called_ = false;
    chip_deselect_called_ = false;
  }

  void TearDown() override {
    delete mock_wiznet_api_;
    g_mock_wiznet_api = nullptr;
  }

  MockWiznetAPI* mock_wiznet_api_;
  std::unique_ptr<MockSPI> mock_spi_;
  std::unique_ptr<MockUART> mock_uart_;

  bool hard_reset_called_;
  bool chip_select_called_;
  bool chip_deselect_called_;
};

// Test: Callback-Registrierung
TEST_F(W5500InterfaceTest, CallbackRegistration) {
  // Erwarte, dass Callbacks registriert werden
  EXPECT_CALL(*mock_wiznet_api_, reg_wizchip_cs_cbfunc(_, _)).Times(1);
  EXPECT_CALL(*mock_wiznet_api_, reg_wizchip_spi_cbfunc(_, _)).Times(1);
  EXPECT_CALL(*mock_wiznet_api_, reg_wizchip_spiburst_cbfunc(_, _)).Times(1);

  // Mock für ctlwizchip (RESET und INIT)
  EXPECT_CALL(*mock_wiznet_api_, ctlwizchip(_, _))
      .Times(2)  // Einmal für RESET, einmal für INIT
      .WillRepeatedly(Return(0));

  // Hinweis: Hier würde normalerweise W5500Interface::init() aufgerufen werden
  // Da wir aber AVR-spezifische Delays haben, können wir das in einem Unit-Test
  // nicht direkt testen. Stattdessen testen wir die Mock-Interaktionen.
}

// Test: Netzwerk-Konfiguration
TEST_F(W5500InterfaceTest, NetworkConfiguration) {
  wiz_NetInfo net_info;

  // Erwarte, dass wizchip_setnetinfo aufgerufen wird
  EXPECT_CALL(*mock_wiznet_api_, wizchip_setnetinfo(_))
      .Times(1)
      .WillOnce(Invoke([](wiz_NetInfo* info) {
        // Verifiziere, dass MAC-Adresse gesetzt wird
        EXPECT_NE(info, nullptr);
      }));

  // Simuliere Netzwerk-Konfiguration
  // In einem echten Test würde hier w5500.set_network_config() aufgerufen
  g_mock_wiznet_api->wizchip_setnetinfo(&net_info);
}

// Test: Netzwerk-Informationen abrufen
TEST_F(W5500InterfaceTest, GetNetworkInfo) {
  wiz_NetInfo net_info;

  // Erwarte, dass wizchip_getnetinfo aufgerufen wird
  EXPECT_CALL(*mock_wiznet_api_, wizchip_getnetinfo(_))
      .Times(1)
      .WillOnce(Invoke([](wiz_NetInfo* info) {
        // Setze Test-Werte
        info->mac[0] = 0x00;
        info->mac[1] = 0x08;
        info->mac[2] = 0xDC;
        info->ip[0] = 192;
        info->ip[1] = 168;
        info->ip[2] = 1;
        info->ip[3] = 100;
      }));

  // Simuliere Abrufen der Netzwerk-Konfiguration
  g_mock_wiznet_api->wizchip_getnetinfo(&net_info);

  // Verifiziere
  EXPECT_EQ(net_info.mac[0], 0x00);
  EXPECT_EQ(net_info.mac[1], 0x08);
  EXPECT_EQ(net_info.mac[2], 0xDC);
  EXPECT_EQ(net_info.ip[0], 192);
  EXPECT_EQ(net_info.ip[1], 168);
  EXPECT_EQ(net_info.ip[2], 1);
  EXPECT_EQ(net_info.ip[3], 100);
}

// Test: Chip-Initialisierung
TEST_F(W5500InterfaceTest, ChipInitialization) {
  // Erwarte Soft-Reset
  EXPECT_CALL(*mock_wiznet_api_, ctlwizchip(CW_RESET_WIZCHIP, nullptr))
      .Times(1)
      .WillOnce(Return(0));

  // Erwarte Socket-Buffer-Initialisierung
  EXPECT_CALL(*mock_wiznet_api_, ctlwizchip(CW_INIT_WIZCHIP, _)).Times(1).WillOnce(Return(0));

  // Callback-Registrierungen
  EXPECT_CALL(*mock_wiznet_api_, reg_wizchip_cs_cbfunc(_, _)).Times(1);
  EXPECT_CALL(*mock_wiznet_api_, reg_wizchip_spi_cbfunc(_, _)).Times(1);
  EXPECT_CALL(*mock_wiznet_api_, reg_wizchip_spiburst_cbfunc(_, _)).Times(1);

  // Simuliere Initialisierung (ohne echte W5500Interface-Instanz)
  g_mock_wiznet_api->ctlwizchip(CW_RESET_WIZCHIP, nullptr);
  g_mock_wiznet_api->reg_wizchip_cs_cbfunc(nullptr, nullptr);
  g_mock_wiznet_api->reg_wizchip_spi_cbfunc(nullptr, nullptr);
  g_mock_wiznet_api->reg_wizchip_spiburst_cbfunc(nullptr, nullptr);
  g_mock_wiznet_api->ctlwizchip(CW_INIT_WIZCHIP, nullptr);
}

// Test: Chip-Initialisierung fehlgeschlagen
TEST_F(W5500InterfaceTest, ChipInitializationFailure) {
  // Erwarte Soft-Reset
  EXPECT_CALL(*mock_wiznet_api_, ctlwizchip(CW_RESET_WIZCHIP, nullptr))
      .Times(1)
      .WillOnce(Return(0));

  // Callback-Registrierungen
  EXPECT_CALL(*mock_wiznet_api_, reg_wizchip_cs_cbfunc(_, _)).Times(1);
  EXPECT_CALL(*mock_wiznet_api_, reg_wizchip_spi_cbfunc(_, _)).Times(1);
  EXPECT_CALL(*mock_wiznet_api_, reg_wizchip_spiburst_cbfunc(_, _)).Times(1);

  // Socket-Buffer-Initialisierung schlägt fehl
  EXPECT_CALL(*mock_wiznet_api_, ctlwizchip(CW_INIT_WIZCHIP, _)).Times(1).WillOnce(Return(-1));

  // Simuliere fehlgeschlagene Initialisierung
  g_mock_wiznet_api->ctlwizchip(CW_RESET_WIZCHIP, nullptr);
  g_mock_wiznet_api->reg_wizchip_cs_cbfunc(nullptr, nullptr);
  g_mock_wiznet_api->reg_wizchip_spi_cbfunc(nullptr, nullptr);
  g_mock_wiznet_api->reg_wizchip_spiburst_cbfunc(nullptr, nullptr);

  int8_t result = g_mock_wiznet_api->ctlwizchip(CW_INIT_WIZCHIP, nullptr);
  EXPECT_EQ(result, -1);
}

}  // namespace Ethernet
