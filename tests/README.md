# Smart Bell - Test Suite

## Übersicht

Dieses Projekt verwendet Google Test (GTest) für Unit-Tests. Die Test-Infrastruktur ist so konfiguriert, dass sie sowohl auf Linux (x86_64) als auch für Cross-Compilation zu AVR läuft.

## Test-Struktur

```
tests/
├── CMakeLists.txt           # Test-Konfiguration
├── Utils/                   # Tests für Utility-Klassen
│   └── CircularBuffer_test.cpp
└── Ethernet/                # Ethernet-Tests (derzeit deaktiviert)
    ├── MockWiznetAPI.h
    ├── MockWiznetAPI.cpp
    ├── EmbeddedSocketInterface_test.cpp
    └── W5500Interface_test.cpp
```

## Aktuelle Tests

### ✅ CircularBuffer Tests
- **Datei**: `tests/Utils/CircularBuffer_test.cpp`
- **Status**: Aktiv und funktionsfähig
- **Tests**: 6 Tests
  - `PushBackAndPopFront`: Basis-Funktionalität
  - `BufferFull`: Voll-Zustand prüfen
  - `FillAndEmptyBuffer`: Kompletter Zyklus
  - `BufferEmpty`: Leer-Zustand prüfen
  - `UsedEntries`: Füllstand-Tracking
  - `ClearBuffer`: Buffer zurücksetzen

### ⚠️ Ethernet Tests (Deaktiviert)
- **Status**: Vorläufig deaktiviert
- **Grund**: Header-Kollisionen zwischen WIZnet-Library und Standard-C-Bibliothek
  - `close()` kollidiert mit POSIX `close(int fd)`
  - WIZnet verwendet Makros statt Funktionen (`getSn_SR`, `setSn_IR`)
  - Google Mock kann Makros nicht direkt mocken
- **Lösung**: Benötigt einen C++ Wrapper um die WIZnet C-API

## Tests ausführen

### Voraussetzungen

1. **Python Virtual Environment aktivieren**:
   ```bash
   cd /home/david/dev/smart-bell
   source .venv/bin/activate
   ```

2. **Conan-Dependencies installieren**:
   ```bash
   conan install . --build=missing -o tests=True -o platform=linux -s build_type=Release
   ```

### Build & Test

```bash
# CMake konfigurieren
cd build/x86_64/Release
cmake ../../../ -DCMAKE_BUILD_TYPE=Release -DENABLE_UNIT_TESTS=ON

# Tests kompilieren
cmake --build . --target run_gTest

# Tests ausführen
./tests/run_gTest

# Oder mit CTest
ctest --output-on-failure
```

### Erwartete Ausgabe

```
[==========] Running 6 tests from 1 test suite.
[----------] 6 tests from CircularBufferTest
[ RUN      ] CircularBufferTest.PushBackAndPopFront
[       OK ] CircularBufferTest.PushBackAndPopFront (0 ms)
...
[  PASSED  ] 6 tests.
```

## Neue Tests hinzufügen

### 1. Test-Datei erstellen

Erstelle eine neue `.cpp`-Datei in dem passenden Verzeichnis:

```cpp
#include <gtest/gtest.h>
#include "YourClass.h"

namespace {

class YourClassTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Setup-Code hier
  }

  void TearDown() override {
    // Cleanup-Code hier
  }
};

TEST_F(YourClassTest, TestName) {
  // Dein Test
  EXPECT_EQ(1, 1);
}

}  // namespace
```

### 2. CMakeLists.txt aktualisieren

Füge deine Test-Datei in `tests/CMakeLists.txt` hinzu:

```cmake
set(TEST_SOURCES_UTILS 
    ${CMAKE_CURRENT_SOURCE_DIR}/Utils/CircularBuffer_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Utils/YourClass_test.cpp  # <-- Neu
)
```

### 3. Neu kompilieren

```bash
cmake --build build/x86_64/Release --target run_gTest
```

## Troubleshooting

### Problem: "No such file or directory" beim Kompilieren

**Lösung**: Prüfe, ob alle Include-Pfade in `CMakeLists.txt` korrekt sind:

```cmake
target_include_directories(${EXECUTABLE_UNIT_TEST} PRIVATE
    ${CMAKE_SOURCE_DIR}/public
    ${CMAKE_SOURCE_DIR}/src
)
```

### Problem: Linker-Fehler

**Lösung**: Stelle sicher, dass die entsprechende Library gelinkt ist:

```cmake
target_link_libraries(${EXECUTABLE_UNIT_TEST}
    GTest::gtest
    GTest::gtest_main
    YourLibrary  # <-- Füge deine Library hinzu
)
```

### Problem: Tests schlagen fehl

**Lösung**: Führe Tests mit mehr Ausgabe aus:

```bash
./tests/run_gTest --gtest_verbose
```

## Zukünftige Verbesserungen

- [ ] C++ Wrapper für WIZnet-API erstellen
- [ ] Ethernet-Tests reaktivieren
- [ ] Code-Coverage-Reporting hinzufügen
- [ ] Integration Tests für Hardware-Komponenten
- [ ] CI/CD Pipeline für automatische Tests

## Referenzen

- [Google Test Documentation](https://google.github.io/googletest/)
- [Google Mock Documentation](https://google.github.io/googletest/gmock_for_dummies.html)
- [CMake Testing](https://cmake.org/cmake/help/latest/manual/ctest.1.html)
