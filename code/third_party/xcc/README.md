# XCC Blowfish

Source: [EA Mission Editor, fixed revision 6abf0f557469baea73079c6bf6550709e2e3584e](https://github.com/electronicarts/CNC_TS_and_RA2_Mission_Editor/tree/6abf0f557469baea73079c6bf6550709e2e3584e/3rdParty/xcc/misc), `blowfish.cpp` and `blowfish.h`.

Copyright Olaf van der Spek. GPL-3.0-or-later; original notices and COPYING retained.

Local modifications: namespace isolation; standard C++20 headers and a fixed 56-byte key span; unsigned key shifts; replaced x86 assembly and unaligned pointer casts with portable big-endian byte reads/writes; reject partial blocks. The P/S constants, key schedule and rounds remain from XCC. No game assets are included.
