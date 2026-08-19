# Vortex Wireless Web UI

Vortex can expose a local Wi-Fi access point and browser chat interface. The intended deployment is a standalone ESP32 AP: connect to `VORTEX-AI`, then open `192.168.4.1` in a browser. No internet or USB cable is required after flashing.

Default AP credentials are configured in `src/vortex_web.cpp`. Change them before production use if desired.

The UI provides chat, SD/model status, IP address, purpose, and Vortex commands. Captive-portal style DNS redirection is included so common mobile devices can reach the UI more easily.
