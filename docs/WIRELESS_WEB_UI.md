# Vortex wireless browser access

The ESP32 CYD can create its own local Wi-Fi network named `VORTEX-AI` and serve a browser chat UI at `192.168.4.1`.

The web server is local-only: client devices do not need internet access. Captive-portal detection endpoints are provided and DNS requests are redirected to the ESP32 so phones that show a captive-portal prompt can reach the Vortex page.

Default network:
- SSID: `VORTEX-AI`
- Password: `Vortex208682De`
- Address: `http://192.168.4.1`

Change the password in `src/vortex_web.cpp` before sharing the firmware widely.
