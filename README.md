# 🔥 Industrial Safety Monitor — IoT Fire & Gas Detection System

**An ESP32-based fire and combustible gas monitoring prototype with real-time alerts, cloud logging, and automatic water-pump activation.**

Built for **CCS 4062 — Embedded Systems and Internet of Things**, Faculty of Computing, University of Sri Jayewardenepura.

**Tech stack:** `ESP32` · `Arduino/C++` · `MQ-2` · `Flame Sensor` · `Blynk IoT` · `Firebase Realtime Database` · `Wi-Fi`

🎥 **[Watch the project demonstration](https://drive.google.com/file/d/1R-cdkUeuNK5ejM0UBoD51DMrxkh0Ifvc/view?usp=sharing)**

## 📖 Overview

Fire and combustible gas leaks can become dangerous before someone notices them. This project uses an **ESP32** to monitor an **MQ-2 gas/smoke sensor** and a **digital flame sensor**. It provides a local audible alarm and remote notifications, records readings for later review, and automatically switches on a small water pump when a flame is detected.

The **Blynk** dashboard displays live sensor values and alarm states, while **Firebase Realtime Database** stores a timestamped history. To reduce false gas alarms, the firmware learns a clean-air baseline during startup rather than relying on a fixed ADC threshold.

> [!WARNING]
> **Educational prototype only.** This system is not a certified fire alarm, gas detector, or fire-suppression device. Do not rely on it to protect people or property, and do not use it as a replacement for approved safety equipment.

## ✨ Features

- 🔍 **Gas/smoke monitoring:** Samples the MQ-2 analog output and averages 16 readings to reduce noise.
- 🎯 **Adaptive gas detection:** Performs a 120-second warm-up and calculates a baseline from the final 20 readings; hysteresis prevents rapid alarm toggling.
- 🔥 **Flame detection:** Reads an active-LOW digital flame sensor.
- 🔊 **Local audible alert:** Activates the buzzer when either gas or flame is detected.
- 💧 **Automatic water-pump response:** Uses a relay to power the pump **only when a flame is detected** (not for gas-only alerts).
- 📱 **Blynk dashboard:** Sends readings and statuses through virtual pins `V0`–`V3`, nominally once per second.
- 🔔 **Event-based notifications:** Sends `gas_alert` and `fire_alert` when each condition first becomes active, rather than repeating every second.
- ☁️ **Firebase logging:** Records gas values and alarm states every 10 reading cycles or whenever gas/flame detection changes.
- 🖥️ **Serial monitoring:** Prints calibration progress, sensor readings, alarms, and Firebase responses.

## 🏗️ System Architecture

```text
 MQ-2 Gas/Smoke Sensor ── analog (GPIO34) ──┐
                                           │
 Flame Sensor ────────── digital (GPIO35) ─┤
                                           ▼
                                         ESP32
                                       /   |   \
                                      /    |    \
                             GPIO26  /  GPIO25   \  2.4 GHz Wi-Fi
                                    ▼      ▼      ▼
                                  Buzzer  Relay  Internet
                                            |      ├── Blynk: live dashboard + alerts
                                         5V Pump   └── Firebase: timestamped readings
```

### Automatic response

| Detected condition | Buzzer | Water pump | Blynk event |
|---|---|---|---|
| No gas or flame | OFF | OFF | None |
| Gas only | ON | OFF | `gas_alert` |
| Flame only | ON | ON | `fire_alert` |
| Gas and flame | ON | ON | Both events (on new detection) |

The pump turns off when the flame sensor no longer reports a flame. Gas detection is disabled during the startup warm-up, but flame detection remains active **after the main loop begins**.

## 🧰 Hardware Requirements

| Component | Quantity | Purpose |
|---|---:|---|
| ESP32 development board | 1 | Sensor processing, output control, Wi-Fi |
| MQ-2 gas/smoke sensor (bare 6-pin type in this prototype) | 1 | Combustible gas and smoke sensing |
| Digital flame sensor module | 1 | Flame detection (active LOW) |
| Buzzer | 1 | Audible alarm |
| 5 V relay module | 1 | Switching the separate pump circuit |
| 5 V DC mini water pump | 1 | Demonstration of automatic water suppression |
| 18650 battery pack / suitable pump power supply | 1 | Separate power source for pump |
| 10 kΩ resistor | 1 | MQ-2 load resistor |
| 100 Ω resistor | 1 | MQ-2 analog signal series resistor |
| Breadboard, jumper wires, USB power | As needed | Prototyping and ESP32 power |

### ESP32 pin mapping

| Device / signal | ESP32 connection | Notes |
|---|---|---|
| MQ-2 analog signal | `GPIO34` | ADC1 input; 12-bit readings (0–4095) |
| Flame sensor digital output (`DO`) | `GPIO35` | `LOW` means flame detected |
| Buzzer positive/control | `GPIO26` | `HIGH` activates buzzer in this sketch |
| Relay control (`IN`) | `GPIO25` | Configured active-HIGH by default |
| Flame sensor `VCC` / `GND` | `3V3` / `GND` | Use a 3.3 V-compatible sensor output |
| Relay module `VCC` / `GND` | `VIN (5V)` / `GND` | Check that the module accepts 3.3 V control logic |

**MQ-2 bare 6-pin wiring used in the project:**

- Connect one **H** (heater) pin to `5V/VIN` and the other **H** pin to `GND`.
- Join the two **A** pins and connect them to `3V3`.
- Join the two **B** pins; connect their junction through a **10 kΩ resistor to GND**, and through a **100 Ω resistor to GPIO34**.
- This arrangement powers the sensing divider from 3.3 V so its output stays within the ESP32 ADC voltage range. **Do not connect a 5 V analog output directly to an ESP32 GPIO.**

**Pump circuit:** Connect battery/supply **+** to relay `COM`, relay `NO` to pump **+**, and pump **−** to battery/supply **−**. The pump must use an appropriately rated **separate power supply**, not an ESP32 GPIO pin. `NC` is unused. Use appropriate suppression for inductive loads and verify current ratings and wiring before powering the circuit.

> [!IMPORTANT]
> The pinout above matches the project's **bare MQ-2 sensor**. Many off-the-shelf MQ-2 *modules* have different terminals (`VCC`, `GND`, `AO`, `DO`); do not copy the bare-sensor wiring onto a module. Verify the specific hardware datasheets, relay polarity, and power requirements before connecting anything. A high-current buzzer also needs a suitable driver instead of direct GPIO drive.

## 🚀 Getting Started

### 1. Install the development tools

1. Install [Arduino IDE](https://www.arduino.cc/en/software).
2. In **Boards Manager**, install **esp32 by Espressif Systems**.
3. In **Library Manager**, install the **Blynk** library.
4. The ESP32 board package supplies the `WiFi.h`, `HTTPClient.h`, and `WiFiClientSecure.h` dependencies used by the sketch.
5. Connect your ESP32 to your computer with a data-capable USB cable.

### 2. Get the source code

```bash
git clone https://github.com/Dulakshi-dev/iot-fire-gas-detection.git
cd iot-fire-gas-detection
```

Open [`gas_fire.ino`](gas_fire.ino) in Arduino IDE. If Arduino IDE asks to create a sketch directory named `gas_fire`, allow it.

### 3. Configure Blynk

1. Create a project/template at [Blynk IoT](https://blynk.io/) for an ESP32 device.
2. Copy your **Template ID**, **Template Name**, and **Device Auth Token**.
3. Create these **virtual pin datastreams** (integer types are suitable):

   | Virtual pin | Value | Suggested widget |
   |---|---|---|
   | `V0` | MQ-2 raw ADC reading (`0`–`4095`) | Gauge / chart |
   | `V1` | Gas detected (`0` or `1`) | LED / indicator |
   | `V2` | Flame detected (`0` or `1`) | LED / indicator |
   | `V3` | Alarm active (`0` or `1`) | LED / alarm indicator |

4. Configure Blynk events with the **exact event codes** `gas_alert` and `fire_alert`, and enable push notifications for the intended users/devices.
5. Add the widgets to your Blynk dashboard and bind each widget to the corresponding datastream.

### 4. Configure Firebase Realtime Database

1. Create a project in the [Firebase Console](https://console.firebase.google.com/).
2. Create a **Realtime Database** (not Firestore).
3. Note your database URL, including its actual regional hostname. The firmware posts JSON to the `/readings.json` endpoint, for example:

   ```cpp
   const char* FIREBASE_URL =
       "https://YOUR-PROJECT-default-rtdb.firebaseio.com/readings.json";
   ```

4. Review the database's access rules before connecting the device. **The current prototype code performs unauthenticated writes**, so a database that requires authentication will reject those POST requests. Use an isolated, controlled test environment if reproducing the original experiment; for a real installation, implement authenticated writes and restrictive rules.

### 5. Set your device configuration

In `gas_fire.ino`, replace the placeholders with values for **your own** Blynk project, Wi-Fi network, and Firebase database:

```cpp
#define BLYNK_TEMPLATE_ID   "YOUR_BLYNK_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "FIRE and GAS Alert"
#define BLYNK_AUTH_TOKEN    "YOUR_BLYNK_TOKEN"

char ssid[] = "YOUR_WIFI_NAME";
char pass[] = "YOUR_WIFI_PASSWORD";

const char* FIREBASE_URL =
    "https://YOUR-PROJECT-default-rtdb.firebaseio.com/readings.json";
```

Use **2.4 GHz Wi-Fi**. Do not commit actual Wi-Fi passwords, Blynk tokens, or database credentials to a public repository; use a local, untracked configuration for real credentials.

If your relay module is active-LOW, adjust this constant to match its actual behavior:

```cpp
const bool RELAY_ACTIVE_LOW = true; // only for active-LOW relay modules
```

### 6. Upload and monitor

1. In Arduino IDE, select the appropriate ESP32 board (for example, **ESP32 Dev Module**) and serial port.
2. Click **Verify**, then **Upload**.
3. Open **Serial Monitor** at **115200 baud**.
4. Power on in **clean air** and allow the MQ-2 to warm up for approximately **120 seconds**.
5. Confirm a message similar to `>>> Gas baseline: ...` appears.
6. Check that live readings appear in Blynk and, with correctly authorized database writes, that timestamped entries appear in Firebase.

Example serial output (illustrative):

```text
Booting...
Connecting to Wi-Fi: YOUR_WIFI_NAME
Wi-Fi connected
Reading #1
Gas: 505 (warming up) | Gas Detected: NO | Flame: NO | Alarm: OFF
...
>>> Gas baseline: 500
Reading #121
Gas: 503 | Gas Detected: NO | Flame: NO | Alarm: OFF
```

## 🧠 Gas Detection Algorithm

The firmware averages **16 analog samples**, with a 5 ms delay between samples. During the first **120 reading cycles**, it learns a clean-air baseline by averaging the final **20 readings**. After that:

```cpp
if (!gasState && gasRaw > gasBaseline + GAS_MARGIN)
    gasState = true;
else if (gasState && gasRaw < gasBaseline + GAS_MARGIN - GAS_HYST)
    gasState = false;
```

| Setting | Value | Effect |
|---|---:|---|
| `WARMUP_SECONDS` | `120` | Disable gas alerts while baseline is learned |
| `BASELINE_SAMPLES` | `20` | Number of end-of-warm-up samples used for calibration |
| `GAS_MARGIN` | `300` | Alarm activates above baseline + 300 ADC counts |
| `GAS_HYST` | `100` | Alarm clears below baseline + 200 ADC counts |

For example, if the learned baseline is `500`, the gas alert **activates above 800** and **clears below 700**. These are **relative ADC readings, not gas concentration in ppm**.

## ☁️ Firebase Data Format

A successful write creates a timestamped entry below `/readings`, containing fields such as:

```json
{
  "gas": 1620,
  "gasAlert": true,
  "flame": false,
  "alarm": true,
  "ts": 1760000000000
}
```

The `ts` field is generated by Firebase using a server-side timestamp placeholder in the POST payload. Records are sent **every 10 reading cycles or immediately on a gas/flame detection state change**. With normal one-second cycles, periodic uploads are approximately every 10 seconds, but network delays can affect the actual timing.

The Firebase implementation currently uses `WiFiClientSecure::setInsecure()`, which **does not validate the server's TLS certificate**. This is a prototype limitation that must be corrected before any production use.

## 🧪 Testing and Demonstration

The group project report documents tests for sensor warm-up, stable clean-air readings, gas detection and clearing, flame detection, pump activation and shutdown, Blynk updates, Firebase logging, and Wi-Fi recovery.

**Observed results in the reported indoor prototype tests:**

| Test observation | Reported result |
|---|---|
| Clean-air gas baseline after warm-up | Approximately `500` ADC counts |
| Peak MQ-2 reading in the gas test | Approximately `2500` ADC counts |
| Gas alert response | Approximately `2` seconds |
| Pump activation after flame detection | Approximately `1` second |
| Blynk notification after flame detection | Approximately `7` seconds |

These are **results from one prototype test setup**, not guaranteed response times. Avoid deliberately releasing combustible gas or introducing an open flame outside a properly controlled, supervised test environment. Prefer safe simulated inputs when verifying application logic.

🎬 **[Project demonstration video](https://drive.google.com/file/d/1R-cdkUeuNK5ejM0UBoD51DMrxkh0Ifvc/view?usp=sharing)** (availability depends on the video's sharing settings).

## 🗂️ Repository Structure

```text
iot-fire-gas-detection/
├── gas_fire.ino   # ESP32 Arduino firmware
└── README.md      # Project documentation
```

## ⚠️ Known Limitations

- The MQ-2 responds to **multiple combustible gases and smoke**; it cannot identify the specific gas or provide calibrated ppm measurements in this implementation.
- **Gas alerts are disabled during startup calibration** (approximately the first 120 seconds after reboot).
- `Blynk.begin()` can block while connecting: **local monitoring and alarms are not guaranteed to run independently of Wi-Fi/Blynk connectivity** in the current firmware.
- HTTPS uploads can delay the nominal one-second loop; the configured HTTP timeout is **3 seconds**.
- TLS certificate verification is disabled for Firebase, and the supplied code has **no authenticated Firebase write flow**.
- The pump runs **only while the flame sensor detects flame**; no minimum pump-run duration or manual override is implemented.
- A single short-range flame sensor has limited viewing coverage; there is no sensor redundancy or backup power provision for the full system.
- The prototype is **not appropriate for safety-critical deployment**.

## 🔮 Future Improvements

- Non-blocking networking so local alarms and pump control remain responsive without internet access.
- Firebase authentication, restrictive database rules, and proper TLS certificate validation.
- Temperature sensing or multiple sensors for improved fire confirmation.
- Manual pump override and a minimum pump activation time.
- Safer actuator drivers, flyback protection, and battery monitoring/backup.
- Additional notification options and a dedicated web analytics dashboard.
- Multiple connected sensor nodes for wider coverage.

## 👥 Project Team

Developed as a group assignment by:

- S. Rushani
- P. M. S. H. Puwakgolla
- I. D. D. C. Gammanpila
- N. W. V. T. Pabasara
- M. A. S. D. Fernando

**Faculty of Computing — University of Sri Jayewardenepura**  
**Module:** CCS 4062, Embedded Systems and Internet of Things

## 📚 References

- [ESP32 Arduino Core documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/)
- [Blynk documentation](https://docs.blynk.io/)
- [Firebase Realtime Database documentation](https://firebase.google.com/docs/database)

---

*Built to explore embedded sensing, cloud-connected monitoring, and automatic actuator control through a practical IoT prototype.*
