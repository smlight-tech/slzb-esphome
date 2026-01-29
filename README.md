# ESPHome Multi-Device Firmware Repository

This repository contains a structured ESPHome project designed to support multiple devices and hardware revisions from a single, maintainable codebase.
The architecture emphasizes clear separation between hardware definitions, low-level hardware handling, reusable logic, and device composition.

---

## Project Structure Overview

```
.
├── devices/      # Device composition files (select hardware defs, HAL, logic)
├── hw_defs/      # Hardware definitions (pins, inversion, revision-specific constants)
├── hal/          # Hardware Abstraction Layer (buses, expanders, low-level wiring)
├── logic/        # Reusable logic blocks (features, actions, controls, sensors)
├── libraries/    # Shared libraries used by logic (e.g. IR, protocol helpers)
├── platform/     # Core platform setup (wifi, diagnostics, bluetooth, external components)
├── docs/         # Architecture and design documentation
└── *.yaml        # Root entry YAMLs per device (build/flash targets)
```

---

## Directory Responsibilities

### devices/
Defines how a specific device firmware is assembled.
Each file:
- Selects a hardware definition from `hw_defs/`
- Includes required HAL buses and expanders
- Enables logic blocks and actions

No low-level pin definitions should live here.

---

### hw_defs/
Pure hardware description layer:
- GPIO pin assignments
- Logic-level inversion (active-low / active-high)
- Board revision and model constants
- Presence flags for optional peripherals

Purpose: keep all hardware differences isolated and readable.

---

### hal/ (Hardware Abstraction Layer)
Contains low-level, hardware-dependent wiring and primitives:
- UART / I2C / SPI buses
- Optional HW flow control variants
- GPIO vs expander-based implementations
- Reset / flash / power control wiring

HAL does not implement business logic.

---

### logic/
Reusable functional and behavioral blocks:
- Features (LEDs, buttons, buzzer, IR, radios, sensors)
- Actions (confirmation, warning, error)
- Control logic that may combine multiple features

Logic is hardware-agnostic and relies on IDs/interfaces provided by HAL.

---

### libraries/
Shared helper libraries used by logic blocks.
Typically protocol-level or domain-specific helpers that are reused across multiple logic modules.

---

### platform/
Common platform configuration shared by all devices:
- ESPHome core configuration
- Wi-Fi / networking
- Diagnostics
- Bluetooth
- External components

---

### docs/
Additional documentation describing architectural decisions and design rules.

---

## Adding a New Device

1. Create or reuse a hardware definition in `hw_defs/`
2. Add a new device composition file in `devices/`
3. Include required HAL buses and logic blocks
4. Build/flash using the corresponding root YAML file

No changes to existing logic or HAL should be required.

---

## Design Goals

- Support many devices and hardware revisions
- Single source of truth for hardware definitions
- Clear separation of concerns
- Minimal duplication
- Easy long-term maintenance and extension

---

## Supported Devices

| Feature | ULTIMA | MRxU | 06xU | SLWF-09U |
|---------|:------------:|:----------:|:----------:|:------------:|
| **MCU** | ESP32-S3 | ESP32-S3 | ESP32-S3 | ESP32-S3 |
| **Flash** | 16MB | 16MB | 16MB | 16MB |
| **PSRAM** | Yes | Yes | Yes | Yes |
| **GPIO LEDs** | 2 | 2 | 2 | 1 |
| **WS2812 RGB** | 12 LEDs | - | - | Yes |
| **Buttons** | 2 | 1 | 1 | 2 |
| **Ethernet** | Yes | Yes | Yes | Yes |
| **UART Radios** | 3 (CC26, EFR32, ZW-800) | 2 (CC26, EFR32) | 1 (CC26/EFR32) | - |
| **Buzzer** | Yes | - | - | - |
| **IR TX** | Yes | - | - | - |
| **IR RX** | Yes | - | - | - |
| **Microphone** | Yes (I2S) | - | - | Yes (I2S) |
| **UPS I2C** | Yes | - | - | - |
| **I2C Expander** | Yes | - | - | - |
| **4G/LTE Addon** | Yes | - | - | - |
| **USB-C CC ADC** | - | Yes | Yes | - |
| **DIY Expansion** | Yes | - | - | Yes |

---

## Usage Examples

### Buzzer / RTTTL Melodies

Devices with a buzzer (e.g., Ultima) support RTTTL melody playback. You can play melodies from Home Assistant.

**Play a custom RTTTL melody:**
```yaml
service: esphome.<device_name>_rtttl_input_set
data:
  value: "mario:d=4,o=5,b=100:16e6,16e6,32p,8e6,16c6,8e6,8g6,8p,8g"
```

**Play a preset melody:**
```yaml
service: esphome.<device_name>_rtttl_preset_set
data:
  option: "Doorbell"
```

Available presets: `Doorbell`, `Notification`, `Alert`, `Success`, `Error`, `Mario`, `Zelda`, `Pacman`, `Star Wars`, `Nokia`

**RTTTL Format:**
```
name:d=duration,o=octave,b=bpm:notes
```

**Resources for RTTTL melodies:**
- [PICAXE RTTTL Collection](https://picaxe.com/rtttl-ringtones-for-tune-command/)
- [Online RTTTL Player/Editor](https://adamonsoon.github.io/rtttl-play/)

---

### WS2812 LED Effects

Devices with WS2812 LEDs (e.g., Ultima) support various light effects controllable from Home Assistant.

**Turn on with effect:**
```yaml
service: light.turn_on
target:
  entity_id: light.<device_name>_ws2812
data:
  effect: "Rainbow"
```

**Available effects:**

| Category | Effects |
|----------|---------|
| Common | Rainbow, Color Wipe, Scan, Twinkle, Random Twinkle, Fireworks, Flicker, Pulse, Strobe |
| Lambda | Fire, FastLED Fire, Confetti, Candy Cane, Meteor, Running Lights, Breathing RGB, Color Chase, Sparkle, Christmas |
| Music Reactive | Music: Grav, Music: Gravicenter, Music: Pixels, Music: DJ Light, Music: Waterfall, and more |

**Quick presets via dropdown:**
```yaml
service: esphome.<device_name>_ws2812_preset_set
data:
  option: "Rainbow"
```

Available presets:

| Category | Presets |
|----------|---------|
| Solid Colors | White, Warm White, Red, Green, Blue, Purple, Cyan, Orange |
| Moods | Night Light, Cozy |
| Effects | Rainbow, Fire, Twinkle, Confetti, Party, Christmas |
| Alerts | Alert |

**Note:** Music reactive effects require the microphone to be enabled via the "Mic Enabled" switch.

**Short notification blinks (status indicators):**
```yaml
service: esphome.<device_name>_ws2812_notify_set
data:
  option: "OK"
```

| Notification | Color | Pattern |
|--------------|-------|---------|
| OK | Green | Double blink |
| Warning | Orange | Triple blink |
| Error | Red | Rapid 5x blink |
| Info | Blue | Single long blink |
| Busy | Yellow | Fade out |
| Ready | Cyan | Pulse up then off |
| Attention | Magenta | Double flash |
| Boot | White | Sweep fade |

---

### Microphone / Sound Level

Devices with a microphone (e.g., Ultima) expose sound level sensors.

**Enable microphone:**
```yaml
service: switch.turn_on
target:
  entity_id: switch.<device_name>_mic_enabled
```

**Sensors available:**
- `sensor.<device_name>_mic_rms` - RMS sound level
- `sensor.<device_name>_mic_peak` - Peak sound level

**Warning:** The microphone consumes a lot of CPU and memory resources. It is not recommended to use it simultaneously with Zigbee/Thread/Z-Wave UART-to-Ethernet connections, as it may cause instability or packet loss on those interfaces.

---

### IR Remote (Transmit)

Devices with IR transmitter can send IR codes to control TVs, ACs, etc.

**Send a raw IR code:**
```yaml
service: esphome.<device_name>_ir_send
data:
  code: "0x20DF10EF"  # Example: LG TV Power
```

Check `libraries/ir/codes/` for available IR code packs.
