VORTEX AI V3.1 SD CARD

Create this folder structure on the SD card:

/vortex/model.bin
/vortex/tokenizer.bin
/vortex/purpose.txt
/vortex/knowledge/physics/
/vortex/knowledge/math/
/vortex/knowledge/programming/
/vortex/knowledge/esp32/
/vortex/knowledge/godot/
/vortex/knowledge/roblox/
/vortex/knowledge/general/
/vortex/memory/learned.txt

Put UTF-8 .txt or .md knowledge files in the knowledge folders.

Runtime commands:
/learn <text>
/remember <text>
/forget <text>
/knowledge <question>
/reload
/memory
/purpose
208682De

Learning is retrieval-based. It does not rewrite model.bin on the CYD.
