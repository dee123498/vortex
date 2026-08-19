# Vortex AI v3 model

The supplied `model.bin` is a compact trained byte-level transformer checkpoint compatible with `vortex3_fixed.ino`.

Architecture:
- vocabulary: 256 byte tokens
- dimension: 32
- hidden dimension: 64
- layers: 1
- heads: 1
- context: 64 tokens
- checkpoint size: about 105 KB

The training corpus is Vortex-specific and covers:
1. Physics
2. Math
3. Programming
4. ESP32 / CYD / Arduino
5. Godot
6. Roblox / Luau
7. General conversation
8. Vortex identity and purpose

The production firmware also uses SD-card retrieval so Vortex can learn from approved files without changing model weights.

## SD layout

```text
/vortex/model.bin
/vortex/tokenizer.bin
/vortex/purpose.txt
/vortex/knowledge/physics/*.txt
/vortex/knowledge/math/*.txt
/vortex/knowledge/programming/*.txt
/vortex/knowledge/esp32/*.txt
/vortex/knowledge/godot/*.txt
/vortex/knowledge/roblox/*.txt
/vortex/knowledge/general/*.txt
/vortex/memory/*.txt
```

## Purpose command

Send `208682De` in the Serial Monitor. Vortex asks for its new purpose. The next message is saved to `/vortex/purpose.txt` and becomes the active purpose after reboot.

The compact model is trained; the `model.int8.bin` export is provided separately as the quantized artifact for the planned int8 inference backend. The current compatible CYD firmware uses the float32 `model.bin` because its loader has not yet been switched to int8 dequantization.
