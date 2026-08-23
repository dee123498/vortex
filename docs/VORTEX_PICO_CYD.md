# Vortex Pico + CYD Architecture

This branch is the two-board Vortex build. The Arduino Uno is intentionally not used.

## Hardware

```text
Raspberry Pi Pico                 ESP32-2432S028 CYD
-----------------                 ------------------
GP4 / pin 6 TX  ----------------> GPIO3 RX (P1)
GP5 / pin 7 RX  <---------------- GPIO1 TX (P1)
GND             ----------------- GND
```

Do not connect the P1 VIN pins together. Power the boards independently by USB while testing.

The CYD built-in SD interface remains untouched:

- CS = GPIO5
- SCK = GPIO18
- MISO = GPIO19
- MOSI = GPIO23

The CYD display uses its own SPI pins and the touch controller uses its dedicated touch pins.

## Firmware folders

- `pico/` = Pico AI coprocessor
- `cyd/` = CYD display/touch/SD/UI firmware

They are separate PlatformIO projects so each board can be built and flashed independently.

## Serial protocol

The Pico/CYD link uses newline-delimited UTF-8 frames at 115200 baud.

Examples:

```text
HELLO
USER\tguest\tGuest
PROMPT\thello Vortex
```

Pico replies with:

```text
READY\tPICO_AI\tVORTEX
USER_OK\tguest\tGuest
BEGIN
TEXT\tHello, Guest. I'm ready.
END
```

## User recognition

The hardware shown in this project has no biometric camera or microphone. The implementation therefore identifies users by selectable profiles, not by claiming facial or voice recognition. The CYD stores up to four profile IDs/names in `/vortex/users.csv`; the Pico keeps separate short-term memory for each selected profile.

## Talking

The firmware provides two-way conversational text and a short acknowledgement tone through the CYD audio output. Full natural speech/TTS requires a speech engine and a speaker path; this branch does not falsely claim that the bare CYD can produce unrestricted natural-language speech by itself.

## Model files

The existing SD-card model layout remains supported for the CYD:

```text
/vortex/model.bin
/vortex/tokenizer.bin
/vortex/knowledge/
```

The Pico does not load the large SD model because the RP2040 only has 264 KB RAM. The branch deliberately avoids a fake "loaded LLM" implementation. A future tiny quantized RP2040 model can be connected to the same `PROMPT/BEGIN/TEXT/END` protocol without changing the CYD UI.

## Build

Use PlatformIO:

```text
cd pico
pio run
pio run -t upload

cd ../cyd
pio run
pio run -t upload
```

Flash the Pico first, then the CYD. During CYD USB flashing, disconnect the Pico UART from P1 if serial traffic prevents reliable upload.
