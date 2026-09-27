#!/usr/bin/env python3
"""Check visible power pixels through normal Godot startup (requires Pillow).

This supplements the original-machine-code fixtures: it catches a disabled
sidebar in the real host, without writing any power or sidebar state in tests.
It is not a full screenshot comparison against the original game.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

from PIL import Image


def inspect_bar(path, side):
    with Image.open(path) as source:
        image = source.convert('RGB')
    # Original fixed sidebar geometry, not read back from the tested layout.
    x = image.width - 168 + (0 if side else 5)
    bottom = 227 + 50 * ((image.height - 227 - (18 if side else 26) - 7) // 50)
    count = 0
    # Include the final pip's glyph below its anchor. A map without a base
    # can legitimately have only that one pip; clipping it hides most pixels.
    for y in range(227, bottom + 6):
        for px in range(x, x + 12):
            r, g, b = image.getpixel((px, y))
            red = r > 70 and r > g * 1.5 and r > b * 1.5
            green = g > 70 and g > r * 1.5 and g > b * 1.5
            yellow = min(r, g) > 70 and min(r, g) > b * 2
            count += red or green or yellow
    return {'path': str(path), 'width': image.width, 'height': image.height,
            'side': side, 'colored_power_pixels': count, 'passed': count >= 12}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--godot', default='godot')
    parser.add_argument('--game-data', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    project = Path(__file__).resolve().parents[1]
    start = {'action': 'press', 'node': 'MainMenu/Buttons/StartButton'}
    drawn = {'action': 'wait_map', 'state': 'drawn'}
    settle = {'action': 'frames', 'count': 240}
    free_radar = {'action': 'assert', 'field': 'map.radar_available', 'equals': True}
    menu = [{'action': 'key', 'key': 'Escape'},
            {'action': 'press', 'node': 'PauseMenu/Panel/Buttons/MainMenuButton'}]
    captures = []

    def capture(name, side):
        path = output / (name + '.png')
        captures.append((path, side))
        return {'action': 'capture', 'node': 'GameScreen/WorldViewport',
                'path': str(path), 'min_colors': 32}

    steps = [{'action': 'wait_boot'}, {'action': 'set_map', 'map': 'ALL01UMD.MAP'}, start, drawn, settle, free_radar, capture('allied', 0),
             {'action': 'resize', 'width': 1600, 'height': 900},
             {'action': 'wait_map', 'state': 'drawn', 'width': 1600, 'height': 900},
             settle, free_radar, capture('resized', 0), *menu,
             {'action': 'set_map', 'map': 'SOV02SMD.MAP'}, start, drawn, settle,
             # The current map browser retains its default Allied UI skin
             # on this snow map. This exercises map switching, not side setup.
             capture('soviet', 0), *menu,
             {'action': 'set_map', 'map': 'ALL01UMD.MAP'}, start, drawn, settle, free_radar,
             capture('reloaded', 0), *menu]
    script, report = output / 'power.json', output / 'power-report.json'
    script.write_text(json.dumps({'steps': steps}, indent=2) + '\n')
    report.unlink(missing_ok=True)
    env = os.environ.copy()
    env.pop('RA2_GAME_DATA', None)
    with tempfile.TemporaryDirectory(prefix='ra2-power-visible-') as temporary:
        with (output / 'power.log').open('w') as log:
            process = subprocess.run([args.godot, '--path', str(project), '--rendering-method', 'mobile', '--',
                '--game-data=' + str(args.game_data.resolve()),
                '--display-config=' + str(Path(temporary) / 'display.cfg'),
                '--resource-config=' + str(Path(temporary) / 'resources.cfg'),
                '--automation=' + str(script), '--automation-report=' + str(report)],
                cwd=temporary, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=150)
    result = json.loads(report.read_text()) if report.exists() else {}
    assert process.returncode == 0 and result.get('passed'), f'See {output / "power.log"}'
    assert 'ERROR:' not in (output / 'power.log').read_text()
    pixels = [inspect_bar(path, side) for path, side in captures]
    (output / 'power-pixels.json').write_text(json.dumps(pixels, indent=2) + '\n')
    assert all(item['passed'] for item in pixels), pixels
    print(json.dumps(pixels, indent=2))


if __name__ == '__main__':
    main()
