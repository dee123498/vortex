# Vortex CYD Front-End

Target: ESP32-2432S028 2.8-inch CYD with ILI9341 display, XPT2046 touch and built-in microSD.

## Responsibilities

- Vortex touchscreen UI
- on-screen keyboard
- four selectable user profiles
- SD-card user profile storage
- SD model/tokenizer presence detection
- Wi-Fi connection from `/vortex/wifi.txt`
- UART link to the Pico AI coprocessor
- short audio acknowledgement through the CYD audio output

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
```

`wifi.txt` is two lines: SSID on line 1 and password on line 2.

The SD card is storage. It is not extra RAM for the Pico.
