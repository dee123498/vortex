# Vortex Wireless Mode

Vortex can run as its own Wi-Fi access point.

1. Flash the Vortex wireless firmware.
2. Power the CYD from its battery/USB power source; the USB cable is not needed for the client device.
3. On a phone, tablet, or PC, join **VORTEX-AI**.
4. Password: **Vortex208682De**.
5. Open `http://192.168.4.1` in a browser.
6. Chat with Vortex through the local web UI.

No internet connection is required. The web UI is served directly by the ESP32.

The firmware also uses DNS redirection for common captive-portal checks. If a device does not automatically open the page, manually browse to `192.168.4.1`.

For production use, change the default AP password in `src/vortex_web.cpp`.
