# Vortex V4 Web Roles

The ESP32 hosts a local web dashboard at the SoftAP address printed to Serial at boot.

## Roles

### Admin — Dezavious Ojelade
- Username: `dezavious`
- First-run password is generated on the ESP32 and printed to Serial once.
- Change Vortex purpose.
- Run the safe built-in commands (`status`, `reboot`).
- Change the Director password.
- Add names to the command registry.
- Full dashboard/settings access.

### Director
- Username: `director`
- Initial password: `director`; Admin should change it before sharing access.
- On first login, Vortex asks the Director for their name and stores it.
- Can chat with Vortex.
- Can change supported user settings such as browser voice rate.
- Cannot change Vortex purpose, manage users, or execute Admin commands.

### Guest / sub-user
- No password required; enters a display name.
- Can chat with Vortex and use browser voice input/output.
- Cannot run commands, change settings, change purpose, or manage users.

## Voice

The web UI uses the browser's Web Speech APIs. Speech output uses `speechSynthesis`; speech input uses `SpeechRecognition`/`webkitSpeechRecognition` where the browser provides it. Browser support and security policies can vary.

## Security note

The ESP32 SoftAP is intended for local/LAN use. The authentication implementation is a lightweight device-local gate, not a substitute for TLS, a hardened password database, or an internet-facing identity provider. Do not expose the ESP32 directly to the public Internet.

Arbitrary shell/code execution is intentionally not exposed. Admin commands are explicitly registered safe operations so a compromised browser session cannot turn the web UI into a general remote-code-execution endpoint.
