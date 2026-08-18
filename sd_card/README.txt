VORTEX SD-CARD AI V2

Format this card as FAT32.
Create:

/vortex/model.bin
/vortex/tokenizer.bin
/vortex/config.txt       optional
/vortex/wifi.txt         optional
/vortex/knowledge/       optional folder for .txt/.md files

model.bin and tokenizer.bin must use the format documented in:
docs/VORTEX_SD_AI_V2.md

Do NOT put a random .bin file here. The model must match the Vortex transformer format and tokenizer vocabulary size.

The 128 GB capacity is more than enough for storage, but the ESP32's RAM is the actual inference limit.
