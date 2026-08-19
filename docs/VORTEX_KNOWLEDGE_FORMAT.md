# Vortex SD-card knowledge format

Place approved offline knowledge files under `/vortex/knowledge/`.

Recommended layout:

```text
/vortex/knowledge/physics/*.txt
/vortex/knowledge/math/*.txt
/vortex/knowledge/programming/*.txt
/vortex/knowledge/esp32/*.txt
/vortex/knowledge/godot/*.txt
/vortex/knowledge/roblox/*.txt
/vortex/knowledge/general/*.txt
/vortex/memory/*.txt
```

Vortex v3 uses these files as retrieval context. This lets the user add or update knowledge without reflashing the firmware or retraining the neural weights.

Keep files concise because the CYD has limited RAM. Plain UTF-8 text is preferred.

The model should never modify model weights directly from an SD-card file. Learning means indexing/retrieving the file content unless a separately built trainer exports a new checkpoint.
