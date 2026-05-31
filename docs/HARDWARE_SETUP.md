# SmartBell Hardware Setup

## Übersicht

SmartBell ist ein IoT-Türklingel-System basierend auf ATmega328P (Arduino UNO) und W5500 Ethernet-Modul. Das System kommuniziert über MQTT mit einem Smart-Home-Server und kann per PoE (Power over Ethernet) versorgt werden.

## Komponenten

| Komponente | Beschreibung |
|------------|--------------|
| Arduino UNO / ATmega328P | Mikrocontroller @ 16MHz |
| W5500 Ethernet-Modul | 10/100 Mbit Ethernet mit Hardware TCP/IP Stack |
| PoE Splitter (802.3af) | Extrahiert 5V/12V aus PoE |
| Türklingel-Taster | Taster mit Pull-Up (an INT0/PD2) |
| Optional: LED | Status-Anzeige |

---

## Pin-Belegung Arduino UNO ↔ W5500

### SPI-Verbindung

| Arduino UNO Pin | W5500 Pin | Funktion | Beschreibung |
|-----------------|-----------|----------|--------------|
| D13 (PB5) | SCLK | SPI Clock | SPI Takt |
| D12 (PB4) | MISO | SPI MISO | Master In, Slave Out |
| D11 (PB3) | MOSI | SPI MOSI | Master Out, Slave In |
| D10 (PB2) | SCS | Chip Select | SPI Chip Select (active LOW) |

### Steuer-Signale

| Arduino UNO Pin | W5500 Pin | Funktion | Beschreibung |
|-----------------|-----------|----------|--------------|
| D9 (PB1) | RSTn | Reset | Hardware Reset (active LOW) |
| D2 (PD2) | INTn | Interrupt | W5500 Interrupt (optional, active LOW) |

### Stromversorgung W5500

| W5500 Pin | Verbindung | Beschreibung |
|-----------|------------|--------------|
| VCC | 3.3V | 3.3V Versorgung (NICHT 5V!) |
| GND | GND | Masse |

> ⚠️ **WICHTIG**: Der W5500 arbeitet mit 3.3V Logik. Viele W5500-Module haben bereits einen 3.3V Spannungsregler und 5V-tolerante Level-Shifter integriert. Prüfe das Datenblatt deines Moduls!

---

## Verdrahtungsdiagramm

```
                    Arduino UNO                          W5500 Modul
                   ┌───────────┐                        ┌───────────┐
                   │           │                        │           │
     Türklingel ──►│ D2 (INT0) │                        │    RJ45   │◄── Ethernet/PoE
                   │           │                        │           │
                   │ D9        │──── Reset ────────────►│ RSTn      │
                   │ D10       │──── CS ───────────────►│ SCS       │
                   │ D11       │──── MOSI ─────────────►│ MOSI      │
                   │ D12       │◄─── MISO ─────────────│ MISO      │
                   │ D13       │──── SCK ──────────────►│ SCLK      │
                   │           │                        │           │
                   │ 3.3V      │──── VCC ──────────────►│ VCC       │
                   │ GND       │──── GND ──────────────►│ GND       │
                   │           │                        │           │
    PoE 5V ───────►│ VIN       │                        │           │
    PoE GND ──────►│ GND       │                        │           │
                   └───────────┘                        └───────────┘
```

---

## PoE (Power over Ethernet) Setup

### Option 1: Externer PoE Splitter (Empfohlen)

Ein 802.3af/at PoE Splitter wird zwischen dem Ethernet-Kabel und dem W5500-Modul geschaltet.

```
                                    ┌─────────────────┐
    PoE Switch ════════════════════►│  PoE Splitter   │
    (802.3af/at)                    │  (z.B. 5V/2A)   │
                                    └────────┬────────┘
                                             │
                              ┌──────────────┼──────────────┐
                              │              │              │
                              ▼              ▼              ▼
                         ┌────────┐    ┌──────────┐   ┌──────────┐
                         │  5V    │    │   GND    │   │ Ethernet │
                         └───┬────┘    └────┬─────┘   └────┬─────┘
                             │              │              │
                             ▼              ▼              ▼
                    Arduino VIN        Arduino GND    W5500 RJ45
```

**Empfohlene PoE Splitter:**
- 802.3af Splitter mit 5V/2A Ausgang
- Beispiel: TP-Link TL-POE10R (12V) + 12V→5V Buck Converter
- Oder: Passive PoE Splitter (für Passive PoE Injector)

### Option 2: W5500-Modul mit integriertem PoE

Einige W5500-Module haben bereits einen PoE-Splitter integriert:

| Modul | PoE Standard | Ausgangsspannung |
|-------|--------------|------------------|
| W5500-EVB-Pico-PoE | 802.3af | 3.3V (nur für RP2040) |
| WIZ850io + PoE HAT | 802.3af | 5V |

### Verkabelung mit PoE Splitter

```
┌────────────────────────────────────────────────────────────┐
│                        PoE Splitter                        │
│                                                            │
│   RJ45 IN ──────┬──────────────────────── RJ45 OUT         │
│   (vom Switch)  │                        (zum W5500)       │
│                 │                                          │
│                 ▼                                          │
│         ┌─────────────┐                                    │
│         │ PoE Extract │                                    │
│         │  (802.3af)  │                                    │
│         └──────┬──────┘                                    │
│                │                                           │
│           ┌────┴────┐                                      │
│           ▼         ▼                                      │
│        +5V DC     GND                                      │
└────────────────────────────────────────────────────────────┘
              │         │
              │         │
              ▼         ▼
        Arduino VIN   Arduino GND
```

---

## Türklingel-Anschluss

Der Türklingel-Taster wird an INT0 (Pin D2) angeschlossen:

```
        VCC (5V)
           │
           ├──────┐
           │      │
          ┌┴┐    ┌┴┐
          │ │    │ │ 10kΩ Pull-Up
          │ │    │ │ (intern oder extern)
          └┬┘    └┬┘
           │      │
           │      ├─────────► D2 (INT0) Arduino
           │      │
          ┌┴┐     │
          │ │ Taster
          └┬┘     │
           │      │
           └──────┘
           │
          GND
```

> **Hinweis**: Der ATmega328P hat interne Pull-Up Widerstände die im Code aktiviert werden können. Bei langen Kabelwegen (>2m) empfiehlt sich ein externer 10kΩ Pull-Up.

---

## Vollständiger Schaltplan

```
                                                              Ethernet + PoE
                                                                    │
┌─────────────────────────────────────────────────────────────────────────────────┐
│                                                                   │             │
│    ┌──────────────────┐        ┌──────────────────┐        ┌──────┴──────┐      │
│    │    Arduino UNO   │        │   PoE Splitter   │        │  W5500 Modul │      │
│    │                  │        │                  │        │              │      │
│    │              VIN ◄────────┤ +5V              │        │         RJ45 ◄──────┤
│    │              GND ◄────────┤ GND         RJ45 ├────────► RJ45         │      │
│    │                  │        │                  │        │              │      │
│    │              D13 ├────────┼──────────────────┼────────► SCLK         │      │
│    │              D12 ◄────────┼──────────────────┼────────┤ MISO         │      │
│    │              D11 ├────────┼──────────────────┼────────► MOSI         │      │
│    │              D10 ├────────┼──────────────────┼────────► SCS          │      │
│    │               D9 ├────────┼──────────────────┼────────► RSTn         │      │
│    │                  │        │                  │        │              │      │
│    │             3.3V ├────────┼──────────────────┼────────► VCC (3.3V)   │      │
│    │              GND ├────────┼──────────────────┼────────► GND          │      │
│    │                  │        │                  │        │              │      │
│    │               D2 ◄───┐    │                  │        └──────────────┘      │
│    │                  │   │    └──────────────────┘                              │
│    │                  │   │                                                      │
│    │                  │   │    ┌──────────┐                                      │
│    │                  │   └────┤  Taster  ├─────── GND                           │
│    │                  │        │ Klingel  │                                      │
│    └──────────────────┘        └──────────┘                                      │
│                                                                                  │
└──────────────────────────────────────────────────────────────────────────────────┘
```

---

## Pin-Zusammenfassung

| Arduino Pin | ATmega328P Pin | Funktion | Richtung |
|-------------|----------------|----------|----------|
| D2 | PD2 (INT0) | Türklingel-Interrupt | Eingang |
| D9 | PB1 | W5500 Reset | Ausgang |
| D10 | PB2 (SS) | W5500 SPI Chip Select | Ausgang |
| D11 | PB3 (MOSI) | SPI MOSI | Ausgang |
| D12 | PB4 (MISO) | SPI MISO | Eingang |
| D13 | PB5 (SCK) | SPI Clock | Ausgang |
| VIN | - | 5V Versorgung (von PoE) | Power |
| 3.3V | - | 3.3V für W5500 | Power |
| GND | - | Masse | Power |

---

## Software-Konfiguration

### UART Befehle für die Erstkonfiguration

Verbinde einen USB-Serial-Adapter und öffne ein Terminal mit 9600 Baud:

```bash
# IP-Konfiguration
set ip 192.168.1.100
set subnet 255.255.255.0
set gw 192.168.1.1

# MQTT Broker
set broker 192.168.1.50
set port 1883
set user mqttuser
set pass geheim
set id smartbell01

# Konfiguration anzeigen
show

# In EEPROM speichern
save

# Neustart
reboot
```

### MQTT Topics

| Topic | Richtung | Beschreibung |
|-------|----------|--------------|
| `smartbell/ring` | Publish | Klingel-Event `{"event":"ring","count":1}` |
| `smartbell/status` | Publish | Status `{"state":"enabled"}` |
| `smartbell/control` | Subscribe | Steuerung `enable` / `disable` |

---

## Materialiste

| Anzahl | Komponente | Beispiel |
|--------|------------|----------|
| 1 | Arduino UNO oder ATmega328P-Board | Arduino UNO R3 |
| 1 | W5500 Ethernet-Modul | WIZ850io oder W5500 Lite |
| 1 | PoE Splitter 802.3af → 5V | TP-Link PoE Splitter + Buck |
| 1 | Türklingel-Taster | Momenttaster |
| 1 | 10kΩ Widerstand | Pull-Up (optional) |
| - | Jumper-Kabel | Für Verbindungen |
| 1 | Cat5e/Cat6 Kabel | Ethernet + PoE |

---

## Hinweise

1. **3.3V Logik**: Der W5500 arbeitet mit 3.3V. Die meisten Module haben Level-Shifter integriert.

2. **PoE Leistung**: Der Arduino UNO benötigt ~200mA @ 5V. Ein 802.3af Splitter liefert bis zu 12.95W - mehr als ausreichend.

3. **Interrupt**: Der Türklingel-Interrupt löst auf fallende Flanke (LOW) aus. Software-Debouncing ist implementiert.

4. **Watchdog**: Ein Hardware-Watchdog ist aktiviert (2s Timeout) für automatischen Recovery bei Hängern.

5. **EEPROM**: Die Konfiguration wird im internen EEPROM gespeichert (~100 Bytes) und überlebt Stromausfälle.

---

## Troubleshooting

| Problem | Mögliche Ursache | Lösung |
|---------|------------------|--------|
| Keine Ethernet-Verbindung | SPI-Verkabelung | Prüfe MOSI/MISO/SCK/CS |
| W5500 reagiert nicht | Reset-Pin | Prüfe Reset-Leitung |
| Kein MQTT | Falsche Broker-IP | `show` prüfen, `set broker` |
| Arduino startet nicht | PoE Spannung | Prüfe 5V am VIN |
| Klingel wird nicht erkannt | Interrupt | Prüfe D2 und Pull-Up |

---

*Erstellt: SmartBell Projekt - Static IP Mode für ATmega328P*
