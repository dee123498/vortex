# Vortex AI V3.1 Learning System

V3.1 adds persistent SD-card retrieval and explicit memory commands. It does **not** silently retrain neural weights on the ESP32.

## SD layout

```text
/vortex/
  model.bin
  tokenizer.bin
  purpose.txt
  knowledge/
    physics/
    math/
    programming/
    esp32/
    godot/
    roblox/
    general/
  memory/
    learned.txt
```

## Commands

`/learn <text>` — append an approved fact/lesson to `/vortex/memory/learned.txt`.

`/remember <text>` — save a memory.

`/forget <text>` — remove matching memory lines.

`/knowledge <question>` — retrieve the best matching SD document.

`/reload` — rescan SD knowledge and memory files.

`/memory` — show learning-system status.

`/purpose` — show current Vortex purpose.

`208682De` — enter purpose-edit mode; Vortex asks for a new purpose and stores it in `/vortex/purpose.txt`.

## Learning behavior

V3.1 uses retrieval-augmented memory. New files and explicit memories become searchable without reflashing. This is intentional: changing model weights directly on a small ESP32 is unreliable and can corrupt the model. A desktop trainer can later use the accumulated files to fine-tune a new Vortex checkpoint.

## Recommended files

Use UTF-8 `.txt` or `.md`. Keep individual files reasonably small (roughly 1–10 KB) so Vortex can retrieve them without exhausting RAM.

Example:

```text
/vortex/knowledge/physics/newtons_laws.txt

Newton's second law: F = m*a.
Force is measured in newtons. Mass is measured in kilograms and acceleration in m/s^2.
```

The retrieval layer scores query terms against file contents and returns the strongest matching passage to the response layer.
