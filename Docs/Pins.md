# SmartFuseBox – Pin Assignment Reference

This document covers the pin assignments defined in `Local.h` for each supported board, the hardware constraints that govern which pins are safe to use, and the caveats discovered during bring-up. Read this before wiring a new board or porting to a new target.

---

## Contents

1. [Arduino Uno R4 / R4 Minima](#1-arduino-uno-r4--r4-minima)
2. [ESP32 NodeMCU-32S](#2-esp32-nodemcu-32s)
3. [ESP32 Pin Safety Rules](#3-esp32-pin-safety-rules)
4. [Pins to Avoid on ESP32](#4-pins-to-avoid-on-esp32)
5. [Porting Caveats](#5-porting-caveats)

---

## 1. Arduino Uno R4 / R4 Minima

These are the original reference pin assignments. All digital header pins (D3–D9) and analog header pins used in digital mode (A2–A5 = indices 16–19) are safe for relay and sensor use.

### Relay Pins

| Constant  | Pin | Header location        |
|-----------|-----|------------------------|
| `Relay1`  | 7   | Digital D7             |
| `Relay2`  | 6   | Digital D6             |
| `Relay3`  | 5   | Digital D5             |
| `Relay4`  | 4   | Digital D4             |
| `Relay5`  | 19  | Analog A5 (digital mode) |
| `Relay6`  | 18  | Analog A4 (digital mode) |
| `Relay7`  | 17  | Analog A3 (digital mode) |
| `Relay8`  | 16  | Analog A2 (digital mode) |

### Sensor Pins

| Constant              | Pin | Notes                                         |
|-----------------------|-----|-----------------------------------------------|
| `WaterSensorPin`      | A0  | Analog read                                   |
| `WaterSensorActivePin`| 8   | Pulsed HIGH before each reading (anti-corrosion) |
| `Dht11SensorPin`      | 9   | DHT11 one-wire data                           |
| `LightSensorPin`      | 3   | Digital (on/off) mode by default              |

### SD Card SPI Pins

| Constant         | Pin | SPI Signal  |
|------------------|-----|-------------|
| `SdCardCsPin`    | 10  | Chip Select |
| `SdCardMosiPin`  | 11  | MOSI        |
| `SdCardMisoPin`  | 12  | MISO        |
| `SdCardSckPin`   | 13  | Clock       |

---

## 2. ESP32 NodeMCU-32S

Because ESP32 GPIO numbers do not map to Arduino pin names the same way as R4, all pin assignments are wrapped in `#if defined(ESP32)` / `#else` blocks inside `Local.h`. The values below are safe defaults — **adjust them to match your physical relay board wiring** before deploying.

### Relay Pins

| Constant  | Default GPIO | Notes                              |
|-----------|--------------|------------------------------------|
| `Relay1`  | 32           | Safe output pin                    |
| `Relay2`  | 33           | Safe output pin                    |
| `Relay3`  | 25           | Safe output pin; also DAC1         |
| `Relay4`  | 26           | Safe output pin; also DAC2         |
| `Relay5`  | 27           | Safe output pin                    |
| `Relay6`  | 14           | Safe output pin; boot log on reset — harmless in normal operation |
| `Relay7`  | 12           | Safe output pin; **must be LOW at boot** (strapping pin — see [§4](#4-pins-to-avoid-on-esp32)) |
| `Relay8`  | 13           | Safe output pin                    |

> ⚠️ GPIO12 is a strapping pin. If your relay board pulls it HIGH at power-on, the ESP32 will attempt to boot from a 1.8 V flash voltage and fail. Use GPIO13 or another safe pin as a first choice if this is a concern.

### Sensor Pins

| Constant               | Default GPIO | Notes                                                  |
|------------------------|--------------|--------------------------------------------------------|
| `WaterSensorPin`       | 34           | Input-only ADC1; no internal pull-up/down available    |
| `WaterSensorActivePin` | 35           | Input-only ADC1; no internal pull-up/down available    |
| `Dht11SensorPin`       | 4            | Bidirectional GPIO, safe for one-wire                  |
| `LightSensorPin`       | 36           | Input-only ADC1 (GPIO36 / SVP); analog mode default    |

> ℹ️ `WaterSensorPin` and `WaterSensorActivePin` are assigned to input-only pins (34, 35) because the water sensor only needs to be read, not driven. If `WaterSensorActivePin` needs to be driven HIGH to power the sensor, move it to a bidirectional GPIO such as 21 or 22.

### SD Card SPI Pins

Uses ESP32 VSPI (SPI2) default bus pins.

| Constant         | Default GPIO | SPI Signal  |
|------------------|--------------|-------------|
| `SdCardCsPin`    | 5            | Chip Select |
| `SdCardMosiPin`  | 23           | MOSI        |
| `SdCardMisoPin`  | 19           | MISO        |
| `SdCardSckPin`   | 18           | Clock       |

---

## 3. ESP32 Pin Safety Rules

The ESP32 has several categories of pins that must not be freely assigned to peripherals:

### ADC2 Conflict with WiFi

GPIO pins connected to the ADC2 peripheral **cannot be used for analog reads while WiFi is active**. The WiFi driver takes exclusive control of ADC2. The affected pins are:

`GPIO0, GPIO2, GPIO4, GPIO12, GPIO13, GPIO14, GPIO15, GPIO25, GPIO26, GPIO27`

If you assign a sensor pin to one of these and read it while WiFi is running, the read will return garbage or cause a crash. Use **ADC1 pins** (GPIO32–GPIO39) for sensors wherever possible.

### Input-Only Pins

GPIO34, 35, 36 (SVP), and 39 (SVN) are **input-only** — they have no output driver and no internal pull-up or pull-down resistor. They are fine for reading sensors but **cannot** be used as relay outputs or to drive any load.

---

## 4. Pins to Avoid on ESP32

| GPIO(s)    | Reason                                                                                           |
|------------|--------------------------------------------------------------------------------------------------|
| **6–11**   | ⛔ **Internal SPI flash bus.** Calling `pinMode()` or `digitalWrite()` on any of these pins will immediately crash the chip (hard fault → WDT reset). This is the most dangerous mistake when porting from R4, where pins 6–11 are ordinary digital I/O. |
| **0**      | ⚠️ Strapping pin — must be HIGH at boot for normal flash boot. Avoid as a relay output.          |
| **2**      | ⚠️ Strapping pin — connected to on-board LED on many NodeMCU-32S boards; must be LOW at boot on some modules. |
| **12**     | ⚠️ Strapping pin — sets flash voltage at boot. If held HIGH at power-on, the chip attempts to boot at 1.8 V and will not start. Relay boards that drive outputs HIGH at idle are incompatible with this pin. |
| **15**     | ⚠️ Strapping pin — controls boot log output on UART0. Generally safe after boot but best avoided if stability is critical. |
| **1, 3**   | UART0 TX/RX — used by `Serial` (the USB-UART debug port). Do not reassign unless `Serial` is not in use. |
| **34–39**  | Input-only — cannot drive outputs. Safe for analog/digital sensor reads only.                   |

---

## 5. Porting Caveats

### Flash SPI pins crash the ESP32 immediately

**Symptom:** Continuous `TG1WDT_SYS_RESET` reboot loop with no user output, approximately 1 second after `setup()` is entered.

**Cause:** The original `Local.h` used R4 pin numbers directly with no `#if defined(ESP32)` guard. `Relay2 = 6` mapped directly to ESP32 GPIO6, which is the internal flash SPI clock line. The first call to `pinMode(6, OUTPUT)` in `RelayController::setup()` corrupted the flash bus and caused an immediate hard fault.

**Fix:** All pin constants in `Local.h` are now wrapped in `#if defined(ESP32)` / `#else` blocks. The ESP32 block uses GPIO numbers that are verified safe for output use (32, 33, 25, 26, 27, 14, 12, 13).

**Key lesson:** On R4, pins 4–9 are ordinary digital I/O. On ESP32, pins 6–11 are the internal flash SPI bus. These ranges overlap. **Never share a pin number between an R4 and ESP32 build without a platform guard.**

---

### `WiFi.macAddress()` called before WiFi initialisation

**Symptom:** Incorrect or zero MAC address returned; potential instability during `GenerateDefaultPassword()`.

**Cause:** `WiFi.macAddress()` on ESP32 walks the WiFi event subsystem which may not be initialised before `WiFi.mode()` or `WiFi.begin()` has been called.

**Fix:** On ESP32, the MAC address is now read directly from eFuse using `esp_read_mac(mac, ESP_MAC_WIFI_STA)` from `<esp_mac.h>`. This is always safe to call at any point, with no WiFi initialisation required.

---

### ADC sensor pins and WiFi

If you assign sensor pins to ADC2-capable GPIOs (0, 2, 4, 12–15, 25–27) and WiFi is enabled, analog reads from those pins will fail silently or return garbage once the WiFi driver is running. Assign all analog sensors to ADC1 pins (GPIO32–GPIO39) to avoid this.

---

### Serial1 default pins on ESP32 Arduino core 3.x

On Arduino core 3.x for ESP32, `Serial1` defaults to **GPIO26 (RX) / GPIO27 (TX)** rather than the classic GPIO9/10 used in older core versions. Both GPIO26 and GPIO27 are safe for UART use on NodeMCU-32S. If you need to change them, pass the desired pins to `Serial1.begin(baud, SERIAL_8N1, rxPin, txPin)`.
