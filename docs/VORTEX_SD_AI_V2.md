# Vortex SD-Card AI v2

Target board: **ESP32-2432S028 2.8-inch CYD (ILI9341)**.

## SD-card layout

Format the microSD card as FAT32 and create:

```text
/vortex/
  model.bin
  tokenizer.bin
  config.txt        (optional)
  wifi.txt          (optional)
  /knowledge/       (optional .txt/.md files)
```

The firmware checks `/vortex/model.bin` and `/vortex/tokenizer.bin` during boot. If either is missing, the CYD remains usable and reports that the neural model is not loaded.

## `model.bin` format

`model.bin` is a raw little-endian float32 transformer checkpoint using the layout expected by the Vortex loader (derived from the compact llama2.c-style layout).

Header: seven signed 32-bit integers:

1. `vocab_size`
2. `dim`
3. `hidden_dim`
4. `n_layers`
5. `n_heads`
6. `n_kv_heads`
7. `seq_len`

The header is followed by float32 tensors in this order:

```text
token_embedding_table
rms_att_weight
wq
wk
wv
wo
rms_ffn_weight
w1
w2
w3
rms_final_weight
freq_cis_real
freq_cis_imag
wcls
```

The current firmware uses the same tensor ordering and computes rotary frequencies at runtime.

## `tokenizer.bin` format

```text
uint32 max_token_length
repeat vocab_size times:
    float32 score
    uint32 byte_length
    byte_length bytes: UTF-8 token text
```

The tokenizer vocabulary count must exactly match `model.bin`'s `vocab_size`.

## Important CYD limitation

The common ESP32-2432S028 is an ESP32-WROOM-32 board and normally does **not** provide external PSRAM. The SD card is storage, not RAM. The current v2 loader therefore reads the model into available RAM/PSRAM before inference. A model that is too large will be rejected rather than crashing the display.

For this board, the practical path is a very small quantized/compact model or a streaming/quantized inference engine. A normal multi-million-parameter desktop LLM is not suitable merely because the SD card is 128 GB.

## CYD display and SD pins

```text
LCD SCLK 14
LCD MISO 12
LCD MOSI 13
LCD CS   15
LCD DC    2
LCD BL   21
SD CS     5
Touch CS 33
Touch IRQ 36
```

LCD and SD share the SPI bus; the SD card uses CS 5 while the display uses CS 15.

## Boot status

The firmware shows:

- CYD DISPLAY OK
- SD: OK / MISSING
- Wi-Fi status
- Neural model: LOADED / NOT FOUND
- Tokenizer: LOADED / NOT FOUND

Open the serial monitor at **115200 baud** to send prompts.
