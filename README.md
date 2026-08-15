# Vortex: Offline AI on ESP32-2432S028 (PlatformIO)

## Overview
**Vortex** is a lightweight, offline AI running on an ESP32 microcontroller with WiFi connectivity and SD card storage. Features:
- **Topic Comprehension**: Understands physics, mathematics, AI, memory, energy, and computation
- **WiFi Connectivity**: Connect to networks for weather, web search, and updates
- **Web Integration**:
  - Weather fetching (OpenMeteo API - no key required)
  - Web search capability
  - Firmware updates over-the-air (OTA)
- **SD Card Storage**: Save logs, sessions, search results, and weather data
- **Quantized Neural Network**: int8 weights for minimal memory footprint
- **Zero Dependency Fallback**: Works offline if WiFi unavailable
- **Modular Design**: Easily extensible with camera, microphone, and storage stubs

## Project Structure
```
riftvoid/
├── platformio.ini                  # PlatformIO configuration with WiFi/SD libs
├── src/
│   ├── main.cpp                    # Main Vortex app with WiFi & SD demo
│   ├── tiny_llm.cpp                # Vortex AI with topic comprehension
│   ├── tokenizer.cpp               # Text tokenizer (26+ topic words)
│   ├── wifi_manager.cpp            # WiFi connection & HTTP client
│   ├── sd_manager.cpp              # SD card file operations
│   ├── ota_handler.cpp             # Firmware update handler
│   ├── quantized_model.h           # Placeholder quantized weights
│   └── quant_inference.h           # Quantized inference engine
├── include/
│   ├── tiny_llm.h
│   ├── tokenizer.h
│   ├── wifi_manager.h              # WiFi & HTTP APIs
│   ├── sd_manager.h                # SD card APIs
│   └── ota_handler.h               # OTA update APIs
├── lib/
│   └── peripherals/
│       ├── Camera.h                # Camera stub
│       └── Microphone.h            # Microphone stub
├── tools/
│   └── convert_model.py            # Convert float model → quantized C header
└── README.md
```

## Hardware Requirements
- **ESP32** (or ESP32-S3 for more RAM/PSRAM)
- **SD Card Module** (SPI interface, typically CS on GPIO 5)
- **WiFi**: Built-in to ESP32
- **USB Cable**: For programming and serial monitoring

## WiFi & SD Card Setup

### WiFi Configuration
Edit `src/main.cpp` and replace credentials:
```cpp
const char* SSID = "YOUR_SSID";
const char* PASSWORD = "YOUR_PASSWORD";
```

### SD Card Pinout (ESP32 to SD Module)
| ESP32 | SD Module |
|-------|-----------|
| GPIO 23 | MOSI (DIN) |
| GPIO 19 | MISO (DO) |
| GPIO 18 | CLK (SCK) |
| GPIO 5 | CS (Chip Select) |
| GND | GND |
| 3.3V | VCC |

### Libraries Added
- **WiFi**: Native ESP32 WiFi stack
- **HTTPClient**: HTTP requests (weather, updates)
- **SD**: SD card filesystem
- **Update**: Over-the-air firmware updates

## Build & Flash

### Prerequisites
- Install PlatformIO CLI: https://platformio.org/install/cli
- ESP32 board and USB cable
- SD card module (optional, but recommended)

### Build
```bash
cd d:\riftvoid
platformio run
```

### Upload
```bash
platformio run --target upload
```

### Monitor Serial Output
```bash
platformio device monitor --baud 115200
```

## Expected Serial Output
```
=== Vortex: Offline AI on ESP32 with WiFi & SD Card ===

[SETUP] Initializing SD Card...
[SD] Card initialized successfully
[SD] Card size: 16 MB

[SETUP] Connecting to WiFi...
[WiFi] Attempting to connect to YOUR_SSID
[WiFi] Connected!
[WiFi] IP: 192.168.1.100

[SETUP] WiFi connected - IP: 192.168.1.100
[SETUP] Signal strength: -45 dBm

[DEMO] Fetching weather...
Weather: {"latitude":40.7128,"longitude":-74.006,"current":...}

[DEMO] Performing web search...
Search results for 'ESP32 AI':
1. Result 1 - Found in local cache
2. Result 2 - Retrieved from SD card index
3. Result 3 - Simulated web search

[DEMO] Checking for firmware updates...
Current version: 1.0.0

=== Vortex AI Demo ===

---
Prompt: Hello, who are you?
Vortex: Hello! I'm Vortex, a tiny offline LLM. What can I help you with?

...

=== SD Card Contents ===
[FILE] vortex_log.txt (145 bytes)
[FILE] weather.txt (287 bytes)
[FILE] search_results.txt (156 bytes)
[FILE] ai_session.txt (1024 bytes)

Total storage: 16 MB
```

## Features Explained

### WiFi Connectivity
- **Auto-connect**: Attempts connection on startup
- **Connection timeout**: 10 seconds (20 attempts × 500ms)
- **Signal monitoring**: Displays RSSI (signal strength)
- **Graceful fallback**: Runs offline if WiFi unavailable

### Weather API
- Uses **Open-Meteo** (free, no API key required)
- Returns temperature, weather code, and other metrics
- Results saved to `/weather.txt` on SD card

### Web Search
- Simulated web search (logs to SD card for future enhancement)
- Can be extended with a free search API
- Results cached and indexed on SD card

### OTA Updates
- Firmware version tracking (currently 1.0.0)
- Download binary from update server
- Automatic restart on successful update
- Update logs saved to SD card

### SD Card Storage
- **File operations**: write, read, append, delete
- **Logging**: AI sessions, WiFi events, weather, search results
- **File listing**: View all stored files and sizes
- **Storage info**: Display total card capacity

## SD Card File Structures

### `/vortex_log.txt`
Tracks WiFi connections and system events:
```
WiFi connected at 1234ms
Offline mode initiated at 5678ms
```

### `/weather.txt`
Stores fetched weather data:
```
{"latitude":40.7128,"longitude":-74.006,"current":{...}}
```

### `/search_results.txt`
Caches web search results:
```
Search results for 'ESP32 AI':
1. Result 1 - Found in local cache
...
```

### `/ai_session.txt`
Full transcript of AI conversation:
```
=== Vortex AI Session ===
Q: Hello, who are you?
A: Hello! I'm Vortex...
```

## Vortex Topics Supported
- **Greeting**: hello, hi, hey
- **Identity**: who/what are you
- **Humor**: jokes and funny topics
- **Physics**: gravity, force, motion, light
- **Math**: calculations, equations, primes
- **AI**: neural networks, learning, models
- **Memory**: RAM, storage, cache
- **Energy**: power, batteries, voltage
- **Computation**: algorithms, processors

## Build with Custom Model

### Step 1: Prepare Float Model
```python
import numpy as np
w1 = np.random.randn(16, 8).astype(np.float32)
b1 = np.zeros(16, dtype=np.float32)
w2 = np.random.randn(12, 16).astype(np.float32)
b2 = np.zeros(12, dtype=np.float32)

np.save('w1.npy', w1)
np.save('b1.npy', b1)
np.save('w2.npy', w2)
np.save('b2.npy', b2)
```

### Step 2: Quantize
```bash
cd d:\riftvoid\tools
python convert_model.py --w1 w1.npy --b1 b1.npy --w2 w2.npy --b2 b2.npy --out ../src/quantized_model.h
```

### Step 3: Rebuild
```bash
cd d:\riftvoid
platformio run
```

## Troubleshooting

### SD Card Not Detected
- Check wiring (especially CS pin on GPIO 5)
- Verify card is formatted as FAT32
- Try different SD card (some have compatibility issues)
- Enable debug output in `sd_manager.cpp`

### WiFi Connection Fails
- Double-check SSID and password in `main.cpp`
- Verify WiFi is 2.4 GHz (ESP32 doesn't support 5 GHz)
- Check if router allows MAC address registration
- Move closer to router and check signal strength

### OTA Update Issues
- Ensure update server returns raw binary file (.bin)
- Check firmware size fits in available flash
- Verify Update.begin() doesn't exceed free flash space
- Use `ota_handler.saveUpdateLog()` for error tracking

## Next Steps
- [ ] Integrate TensorFlow Lite Micro
- [ ] Add real web search API (DuckDuckGo, SearXNG)
- [ ] MQTT support for remote control
- [ ] BLE for mobile app integration
- [ ] Camera capture with local processing
- [ ] Microphone input with voice commands
- [ ] HTTPS support with certificate pinning
- [ ] Spiffs filesystem alternative to SD card
