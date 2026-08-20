# Vortex V4 training

This directory defines a reproducible host-side training/export pipeline. The ESP32 is an inference target, not the training machine.

## Dataset format
Use JSONL with one object per line:

```json
{"messages":[{"role":"system","content":"You are Vortex..."},{"role":"user","content":"What is F=ma?"},{"role":"assistant","content":"Force equals mass times acceleration."}]}
```

Recommended dataset groups:
- `identity.jsonl`
- `coding.jsonl`
- `math.jsonl`
- `physics.jsonl`
- `esp32.jsonl`
- `godot.jsonl`
- `roblox.jsonl`
- `survival.jsonl`
- `general.jsonl`

Keep a separate validation set. Do not train on the validation set.

## Training
A real V4 checkpoint must be produced with an actual training/fine-tuning framework on a PC/GPU. The repository does not claim a checkpoint is trained until the training run and evaluation artifacts exist.

## Export
The exporter must write:
- tokenizer.bin
- config.json
- model.bin

The model binary must use a documented versioned header and quantization scheme. The firmware should reject unsupported versions rather than interpreting them as the old V3 float32 layout.
