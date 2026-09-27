#!/usr/bin/env python3
"""Exercise every RGB565 value through the production GPU's neutral line path.

Input channels retain the original 5/6-bit values in their high bits. Neutral
lighting must preserve every RGB565 word, and TestShapeGpuReference checks its
exact normalized RGB8 expansion independently using floating-point rounding.
"""
import argparse
import struct
from pathlib import Path


def generate(output):
    side = 256
    size = side * side
    colors = range(size)
    pixels = [((word >> 11) << 3) | (((word >> 5) & 63) << 10)
              | ((word & 31) << 19) | 0xFF000000 for word in colors]
    parameters = [side, side, side, side, 0, 0, 0, 0, side, side] + [0] * 9 + [4]

    def words(values):
        return struct.pack('<' + 'H' * len(values), *values).hex()

    def dwords(values):
        return struct.pack('<' + 'I' * len(values), *values).hex()

    depth = words([0xFFFF] * size)
    lines = ['SHP_GPU_PACKETS_V1', f'{side} {side}', words([0] * size), depth,
             words([127] * size), words([0] * 256), '1', 'all_rgb565_neutral_line',
             dwords(parameters), dwords(pixels), words(colors), depth]
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text('\n'.join(lines) + '\n')
    print(f'{size} RGB565 colors: exact RGBA8 and depth comparison')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    generate(parser.parse_args().output)
