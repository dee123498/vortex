# Vortex CYD Front-End + Web Gateway

Target: ESP32-2432S028 2.8-inch CYD with ILI9341 display, XPT2046 touch and built-in microSD.

## Responsibilities

- Vortex touchscreen chat UI
- on-screen keyboard
- four selectable user profiles
- SD-card user profile storage
- SD model/tokenizer presence detection
- CYD-hosted Vortex Wi-Fi access point
- browser dashboard matching the Vortex web UI used by earlier iterations
- browser text chat plus browser speech recognition/TTS where supported
- Wi-Fi STA connection from `/vortex/wifi.txt` while keeping the Vortex AP active
- UART link to the Pico AI coprocessor
- short audio acknowledgement through the CYD audio output

## Vortex Wi-Fi signal

The CYD now starts in `WIFI_AP_STA` mode. By default it broadcasts:

```text
SSID:     VORTEX-AI
Password: VortexAI123!
Address:  192.168.4.1
```

Connect a phone, tablet, or computer to `VORTEX-AI`, open `http://192.168.4.1/`, and the Vortex dashboard appears.

To change the AP name/password without recompiling, create `/vortex/ap.txt` on the SD card:

```text
YourNewSSID
YourNewPassword
```

The password must be 8-63 characters for WPA2 AP mode.

If `/vortex/wifi.txt` exists, the CYD also attempts to join that network as a station. The Vortex AP stays available, so the direct CYD UI does not disappear when the router connection changes.

## Browser UI features retained

The hosted dashboard includes the Vortex-style dashboard, user/role selection, chat, browser voice input, browser speech output, memory-clear request, system status, and admin-only restart control. The browser talks to the same Pico AI coprocessor over the CYD UART; it is not a second fake AI.

The CYD does not claim biometric user recognition. Users are identified by their selected profile/name and the Pico keeps separate short-term context for those identities.

## P1 wiring

The CYD P1 connector is `VIN - TX(GPIO1) - RX(GPIO3) - GND`.

- Pico GP4 TX -> CYD RX/GPIO3
- Pico GP5 RX <- CYD TX/GPIO1
- Pico GND -> CYD GND
- Do not connect VIN between the boards.

GPIO1/GPIO3 are also connected to the CYD's USB-UART interface. Disconnect the Pico from P1 when flashing or using the CYD USB serial console if the two serial sources interfere.

## SD card

Use the existing CYD SD reader. The firmware looks for:

```text
/vortex/model.bin
/vortex/tokenizer.bin
/vortex/wifi.txt
/vortex/users.csv
/vortex/ap.txt
```

`wifi.txt` is two lines: SSID on line 1 and password on line 2.

`ap.txt` is two lines: Vortex AP SSID on line 1 and WPA2 password on line 2.

The SD card is storage. It is not extra RAM for the Pico.
