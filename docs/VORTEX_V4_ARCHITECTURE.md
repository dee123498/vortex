# Vortex AI V4 — Real LLM Architecture

V4 replaces the tiny 16-dimensional test transformer with a real small decoder-only Transformer that is trained on a PC/GPU and exported to the ESP32 as a quantized, streamable checkpoint.

## Goals
- Vortex-specific conversational behavior
- Coding: C/C++, Python, GDScript, C#, JavaScript, Luau
- Math and physics fundamentals
- ESP32/CYD/Arduino knowledge
- Godot and Roblox development knowledge
- General conversation
- Safe survival/preparedness knowledge
- Vortex identity and task-assistance purpose
- SD-card retrieval and persistent memory remain separate from model weights

## Important hardware reality
The ESP32-2432S028 cannot train a genuinely large language model locally. Training/fine-tuning happens on a PC/GPU. The CYD performs inference. The 128 GB SD card is storage and does not increase ESP32 RAM.

V4 therefore uses a small quantized model sized for the board's available RAM, with streaming from SD and a bounded context window. Larger checkpoints may be stored on SD but are not automatically runnable on the ESP32.

## Training pipeline
1. Build curated instruction/conversation data from approved Vortex datasets.
2. Start from a small open pretrained causal language model whose license permits the intended use.
3. Supervised fine-tune on Vortex identity, coding, math, physics, ESP32, Godot, Roblox, and safe preparedness examples.
4. Evaluate on held-out prompts.
5. Quantize to an ESP32-compatible integer format.
6. Export tokenizer + model metadata + tensors to a V4 SD-card format.
7. Run host-side golden tests against the exported checkpoint before flashing.
8. Run CYD smoke tests for loading, tokenization, generation, and memory use.

## SD layout
```text
/vortex/v4/model.bin
/vortex/v4/tokenizer.bin
/vortex/v4/config.json
/vortex/knowledge/
vortex/memory/
```

## Purpose protocol
The default purpose is stored outside the model so it can be changed without retraining. `208682De` enters purpose-edit mode; Vortex asks for the new purpose and saves it to `/vortex/purpose.txt`.

## Hidden operating modes
V4 may maintain internal mode state for coding, math, physics, hardware, game development, general conversation, and safe preparedness. These are routing/behavior modes, not covert capabilities, and they do not bypass safety or user control.

## Quality requirement
No V4 binary should be labeled trained unless it has actually been trained/fine-tuned and evaluated. Random or synthetic weights are not acceptable as a production V4 model.
