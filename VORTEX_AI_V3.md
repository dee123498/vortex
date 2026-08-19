# Vortex AI v3

Vortex AI v3 is the offline AI architecture for the ESP32-2432S028 CYD.

## Core knowledge targets
- Physics
- Mathematics
- Programming
- ESP32, Arduino, PlatformIO, and CYD
- Godot and GDScript/C# game development
- Roblox and Luau
- General conversation
- Vortex-specific identity and task assistance

## Vortex purpose
Default purpose:

> I am Vortex, an offline AI assistant whose primary purpose is to help Dzavious Ojelade with tasks, learning, programming, electronics, game development, and general problem solving.

The purpose is stored separately from the model so it can be changed without retraining the neural network.

## Purpose change command
The command `208682De` enters purpose-edit mode.

Expected interaction:

```text
User: 208682De
Vortex: What is my new purpose?
User: [new purpose]
Vortex: My purpose has been updated.
```

The new purpose is saved to `/vortex/purpose.txt` on the SD card and loaded at boot. The command itself should never be treated as ordinary user content.

## SD-card learning
Vortex does not silently retrain its neural weights on arbitrary files. Instead, v3 uses retrieval-augmented memory: files are indexed and relevant excerpts are inserted into the prompt/context at inference time.

Supported folders:

```text
/vortex/knowledge/physics/
/vortex/knowledge/math/
/vortex/knowledge/programming/
/vortex/knowledge/esp32/
/vortex/knowledge/godot/
/vortex/knowledge/roblox/
/vortex/knowledge/general/
/vortex/memory/
```

Supported file types: `.txt`, `.md`, and compact text/JSON knowledge files.

A future desktop trainer can periodically fine-tune a larger source model using approved SD-card datasets, then export a new quantized checkpoint for the CYD.

## Model target
The production checkpoint must be quantized/streamable and designed around the ESP32-2432S028's actual RAM. SD-card capacity is storage, not RAM. The previous random checkpoint is only a loader test and must not be presented as a trained AI.

## Safety/robustness
- Never overwrite the model when learning files.
- Keep purpose, memory, and knowledge separate from model weights.
- Provide a reset command for purpose/memory.
- Validate files before indexing them.
- Limit retrieved context to available RAM.
