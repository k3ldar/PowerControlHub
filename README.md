[📋 Setup Guide](Docs/SETUP.md) | [📋 Commands](Docs/commands.md) | [🔧 Local.h Reference](Docs/Local.md) | [🔬 Sensor Reference](Docs/Sensors.md) | [🔩 Bill of Materials](Docs/BOM.md)
# SmartFuseBox ⚡

A modular, Arduino-powered **12V power distribution and control system** designed for boats and other off-grid applications.  
SmartFuseBox combines fused relays, quick-connect aviation plugs, sensor monitoring, WiFi/BLE/MQTT connectivity, and Arduino-based control into a single 3D-printed enclosure — with a built-in security twist.

---

## 📖 Backstory

I bought a mid-80s boat and wanted to give it a **modern electrical upgrade**.  
The original wiring was dated, messy, and lacked any real security. Since the boat is kept in a marina, theft was also a concern.  

SmartFuseBox was born out of the need for:
- **Reliable 12V power distribution** for navigation lights, bilge pumps, and other onboard systems.  
- **Quick-connect modularity** so I can easily add/remove systems with aviation connectors.  
- **Integrated monitoring** via Arduino sensors (temperature, humidity, light, etc.).  
- **Basic security** through a “poor man’s immobiliser” — the control unit must return a valid 12V feed before the system powers up.  

---

## 🛠 Features

### Power & Control
- **8 fused relays** for distributing 12V power safely.
- **Aviation connectors** for quick, reliable connections to onboard systems.
- **Linked relays** — configure pairs that mirror each other's state.
- **Default relay states** — individual relays can be set to default-on at boot.
- **Immobiliser-style security** — the control unit must return a valid 12V feed to enable the system.
- **3D-printed enclosure** for a compact, customizable housing.

### Sensors
- **DHT11** — temperature and humidity monitoring with failure detection.
- **Water level** — analog sensor with 15-reading rolling average and corrosion-reduction active pin control.
- **Light (LDR)** — day/night detection.
- **GPS (GY-GPS6MV2)** — location, speed, altitude, heading, cumulative distance, and automatic UTC time sync.
- **System diagnostics** — free memory, CPU usage, WiFi/BT state, SD log size, uptime, and warning count.
- **Extensible** — add custom local or remote sensors with minimal boilerplate (see [Docs/Sensors.md](Docs/Sensors.md)).

### Connectivity
- **WiFi** (AP and Client mode) with a full HTTP REST API for relay control, sensor data, configuration, sound, and warnings.
- **Bluetooth BLE** — relay control, sensor streaming, and system metrics via BLE characteristics.
- **MQTT** with **Home Assistant auto-discovery** — sensors and relays appear as HA entities automatically.
- **Serial command protocol** for integration with the companion Boat Control Panel (Arduino Mega + Nextion display).

### Diagnostics & Logging
- **SD card logging** — persistent system logs with configurable SPI speed.
- **Warning system** — 32-bit bitmask of typed warnings propagated between both controllers.
- **RTC support** — DS1302 real-time clock with automatic GPS UTC time synchronisation.
- **CPU usage and free memory monitoring** available via serial, WiFi, and MQTT.

### Boat Control Panel
- **Nextion touchscreen** display for relay switching, sensor readout, and system configuration.
- **COLREGS sound signals** (H0–H12) via a reserved relay-driven horn.
- **Config synchronisation** — BCP and SFB automatically stay in sync over the serial link.

---

## 🧩 Board Support

| Board | WiFi | BLE | MQTT | Notes |
|---|---|---|---|---|
| **Arduino Uno R4 WiFi** | ✅ | ✅ | ✅ | WiFi and BLE are mutually exclusive on this board |
| **Arduino Uno R4 Minima** | ❌ | ❌ | ❌ | Serial and sensor support only |
| **ESP32 NodeMCU-32** | ✅ | ✅ | ✅ | Full feature support |

Board selection is controlled by a single `#define` in `Local.h`.

---

## 🔌 System Architecture

### Physical Units

Two hardware units communicate over a 7-wire serial harness:

| Unit | Hardware | Role |
|---|---|---|
| **Smart Fuse Box (SFB)** | Arduino Uno R4 / ESP32 | Relay control, sensor handling, WiFi, BLE, MQTT, SD logging |
| **Boat Control Panel (BCP)** | Arduino Mega + Nextion | User interface, immobiliser logic, time sync, config management |

The 7-wire harness carries: 12V in, 12V out (immobiliser return), GND, TX (link), RX (link), TX (GPS), RX (GPS).

### Software

The firmware is split between two projects that share a common `Shared/` library via symlinks:

```
SmartFuseBox/      ← Fuse box firmware (relays, sensors, WiFi, BLE, MQTT)
BoatControlPanel/  ← Control panel firmware (Nextion display, immobiliser)
Shared/            ← Common command handlers, sensors, config, utilities
```

The SFB firmware is built around `SmartFuseBoxApp`, which owns all subsystems and wires them together at startup. Communication uses a text-based serial command protocol (`COMMAND:key=value;key=value\n`) over two serial ports — one to the BCP and one to a USB debug monitor.

---

## 🚀 Quick Start

1. **Read the setup guide** → [Docs/SETUP.md](Docs/SETUP.md) — covers prerequisites, symlinks, and building.
2. **Configure your board** → edit `Local.h` to select your board, set pin numbers, relay count, and enable features such as `MQTT_SUPPORT` or `WIFI_SUPPORT`.
3. **Flash the SFB sketch** → open `SmartFuseBox/SmartFuseBox.ino` in the Arduino IDE and upload.
4. **Configure WiFi** → send serial commands (`C11:v=1`, `C13:YourSSID`, `C14:YourPassword`) or use the BCP touchscreen.
5. **Connect to Home Assistant** → enable MQTT (`M0:v=1`), set your broker address (`M1:192.168.x.x`), and enable discovery (`M6:v=1`). All sensors and relays appear automatically.

---

## 🗂️ Documentation

| 🚀 Getting Started | ⚙️ Configuration | 📡 Connectivity |
|:---:|:---:|:---:|
| [📋 Setup Guide](Docs/SETUP.md) | [🔧 Local.h Reference](Docs/Local.md) | [📶 WiFi Architecture](Docs/Wifi.md) |
| [🔩 Bill of Materials](Docs/BOM.md) | [📅 Scheduled Events](Docs/ScheduleEvents.md) | [📡 Bluetooth BLE](Docs/BluetoothBle.md) |
| [🔬 Sensor Reference](Docs/Sensors.md) | [⚠️ Warning System](Docs/Warnings.md) | [🗃️ Message Bus](Docs/MessageBus.md) |

---