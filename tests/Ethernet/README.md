# Ethernet Tests - Deaktiviert

## Status: ⚠️ Vorläufig deaktiviert

Diese Tests sind aufgrund von Header-Kollisionen zwischen der WIZnet ioLibrary und Standard-C-Bibliotheken vorläufig deaktiviert.

## Problembeschreibung

### 1. Namenskollisionen

Die WIZnet Socket-API verwendet Funktionsnamen, die mit POSIX-Funktionen kollidieren:

```c
// WIZnet ioLibrary (socket.h)
int8_t close(uint8_t sn);  // ❌ Kollidiert mit POSIX close(int fd)

// POSIX (unistd.h)
int close(int fd);
```

### 2. Makro-Definitionen

Viele W5500-Funktionen sind als Makros definiert, nicht als Funktionen:

```c
// w5500.h
#define getSn_SR(sn) WIZCHIP_READ(Sn_SR(sn))
#define setSn_IR(sn, ir) WIZCHIP_WRITE(Sn_IR(sn), (ir & 0x1F))
```

Google Mock kann keine Makros mocken, nur echte Funktionen.

### 3. Header-Include-Reihenfolge

Die WIZnet-Header müssen in einer bestimmten Reihenfolge inkludiert werden und sind nicht kompatibel mit Google Mock's Header-Struktur.

## Erstellte Test-Dateien

Folgende Dateien wurden erstellt und sind bereit für zukünftige Verwendung:

```
tests/Ethernet/
├── MockWiznetAPI.h               # Mock-Interface für WIZnet-API
├── MockWiznetAPI.cpp             # C-Wrapper-Implementierung
├── MockSPI.h                     # Mock für SPI/UART
├── TestCompat.h                  # Compatibility-Header für AVR-Makros
├── EmbeddedSocketInterface_test.cpp  # 12 Unit-Tests für Socket-Manager
└── W5500Interface_test.cpp       # Tests für W5500-Initialisierung
```

## Implementierte Tests (deaktiviert)

### EmbeddedSocketInterface Tests

1. **OpenSocketSuccess** - Socket erfolgreich öffnen
2. **OpenSocketFailure** - Fehlerbehandlung beim Öffnen
3. **CloseSocket** - Socket schließen
4. **ListenSocket** - Socket in Listen-Modus
5. **ConnectSocket** - Socket verbinden
6. **DisconnectSocket** - Socket trennen
7. **SendData** - Daten senden
8. **ReceiveData** - Daten empfangen
9. **GetInvalidSocketStatus** - Ungültige Socket-Nummer
10. **SocketStatusAfterMultipleOperations** - Status-Tracking
11. **MultipleSocketsParallel** - Mehrere Sockets gleichzeitig

### W5500Interface Tests

1. **CallbackRegistration** - Callback-Setup
2. **NetworkConfiguration** - Netzwerk-Konfiguration
3. **GetNetworkInfo** - Netzwerk-Info abrufen
4. **ChipInitialization** - Chip-Initialisierung
5. **ChipInitializationFailure** - Fehlerbehandlung

## Lösungsansätze

### Option 1: C++ Wrapper-Layer (Empfohlen)

Erstelle einen dünnen C++-Wrapper um die WIZnet-C-API:

```cpp
// WiznetSocketWrapper.h
class WiznetSocketWrapper {
 public:
  virtual ~WiznetSocketWrapper() = default;
  
  // Umbenennung um Kollisionen zu vermeiden
  virtual int8_t wiznet_socket_open(uint8_t sn, uint8_t protocol, 
                                     uint16_t port, uint8_t flag) = 0;
  virtual int8_t wiznet_socket_close(uint8_t sn) = 0;
  virtual int8_t wiznet_socket_listen(uint8_t sn) = 0;
  // ... weitere Methoden
  
  // Wrapper für Makro-Funktionen
  virtual uint8_t wiznet_get_socket_status(uint8_t sn) = 0;
  virtual void wiznet_set_socket_interrupt(uint8_t sn, uint8_t ir) = 0;
};

// Implementierung
class WiznetSocketImpl : public WiznetSocketWrapper {
 public:
  int8_t wiznet_socket_open(uint8_t sn, uint8_t protocol, 
                             uint16_t port, uint8_t flag) override {
    return socket(sn, protocol, port, flag);
  }
  
  uint8_t wiznet_get_socket_status(uint8_t sn) override {
    return getSn_SR(sn);  // Wrapper um Makro
  }
  // ... weitere Implementierungen
};
```

**Vorteile**:
- Saubere Trennung zwischen C und C++
- Einfach zu mocken
- Bessere Fehlerbehandlung möglich

### Option 2: Conditional Compilation

Verwende Conditional Compilation für Tests:

```cpp
// In Tests
#ifdef UNIT_TEST
  #include "MockWiznetAPI.h"
#else
  #include <Ethernet/socket.h>
  #include <Ethernet/W5500/w5500.h>
#endif
```

**Nachteile**:
- Code-Duplizierung
- Schwieriger zu warten

### Option 3: Integration Tests

Fokus auf Integration Tests mit echter Hardware statt Unit Tests:

**Vorteile**:
- Testet reale Hardware-Interaktion
- Keine Mock-Probleme

**Nachteile**:
- Benötigt Hardware-Setup
- Langsamer
- Schwieriger zu debuggen

## Reaktivierung

Um die Tests zu reaktivieren:

1. **Wrapper-Layer erstellen** (siehe Option 1)

2. **CMakeLists.txt anpassen**:
   ```cmake
   # In tests/CMakeLists.txt
   set(TEST_SOURCES_ETHERNET 
       ${CMAKE_CURRENT_SOURCE_DIR}/Ethernet/MockWiznetAPI.cpp
       ${CMAKE_CURRENT_SOURCE_DIR}/Ethernet/EmbeddedSocketInterface_test.cpp
       ${CMAKE_CURRENT_SOURCE_DIR}/Ethernet/W5500Interface_test.cpp
   )
   
   set(TEST_SOURCES_ALL ${TEST_SOURCES_UTILS} ${TEST_SOURCES_ETHERNET})
   ```

3. **EmbeddedSocketInterface anpassen**:
   - Verwende den Wrapper statt direkter WIZnet-API-Aufrufe
   - Injiziere den Wrapper über Konstruktor

4. **Tests kompilieren**:
   ```bash
   cmake --build build/x86_64/Release --target run_gTest
   ```

## Weitere Informationen

- WIZnet ioLibrary: `/extern/ioLibrary_Driver/ioLibrary_Driver-3.2.0/`
- Google Test Docs: https://google.github.io/googletest/
- Google Mock Docs: https://google.github.io/googletest/gmock_for_dummies.html

## Kontakt

Bei Fragen zur Reaktivierung der Tests, siehe Haupt-README oder erstelle ein Issue.
