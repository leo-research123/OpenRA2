#!/usr/bin/env python3
"""Exercise mouse navigation through the real window GUI in development or export."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--godot', default='godot')
    parser.add_argument('--app', type=Path)
    parser.add_argument('--game-data', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    project = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    command = [args.godot, '--path', str(project), '--rendering-method', 'mobile']
    if args.app:
        app = args.app.resolve()
        command = [str(app / 'Contents/MacOS/ra2opengodot' if app.suffix == '.app' else app)]
    def move(x, y): return {'action': 'mouse_move', 'x': x, 'y': y}
    def button(pressed): return {'action': 'mouse_button', 'button': 'Right', 'pressed': pressed}
    def capture(name): return {'action': 'capture', 'node': 'GameScreen/WorldViewport',
                              'path': str(output / (name + '.png')), 'min_colors': 32}
    escape = {'action': 'key', 'key': 'Escape'}
    wait = {'action': 'wait_map', 'state': 'drawn'}
    steps = [{'action': 'wait_boot'}, {'action': 'set_map', 'map': 'ALL01UMD.MAP'},
             {'action': 'press', 'node': 'MainMenu/Buttons/StartButton'}, wait,
        move(640, 360), capture('before'), button(True), move(720, 320), button(False), wait, capture('panned'),
        escape, {'action': 'assert', 'field': 'paused', 'equals': True},
        move(1, 360), button(True), move(20, 350), button(False), {'action': 'frames', 'count': 8},
        {'action': 'assert', 'field': 'paused', 'equals': True}, escape,
        move(720, 320), button(True), move(640, 360), button(False), wait, capture('returned'),
        escape, {'action': 'press', 'node': 'PauseMenu/Panel/Buttons/MainMenuButton'},
        {'action': 'assert', 'field': 'map.state', 'equals': 'empty'}]
    script, report = output / 'camera.json', output / 'camera-report.json'
    script.write_text(json.dumps({'steps': steps}, indent=2) + '\n')
    report.unlink(missing_ok=True)
    env = os.environ.copy()
    env.pop('RA2_GAME_DATA', None)
    with tempfile.TemporaryDirectory(prefix='ra2-camera-') as temporary:
        with (output / 'camera.log').open('w') as log:
            process = subprocess.run(command + ['--', '--game-data=' + str(args.game_data.resolve()),
                '--resource-config=' + str(Path(temporary) / 'resources.cfg'),
                '--display-config=' + str(Path(temporary) / 'display.cfg'),
                '--automation=' + str(script), '--automation-report=' + str(report)], cwd=temporary,
                env=env, stdout=log, stderr=subprocess.STDOUT, timeout=120)
    result = json.loads(report.read_text()) if report.exists() else {}
    assert process.returncode == 0 and result.get('passed'), f'See {report} and {output / "camera.log"}'
    text = (output / 'camera.log').read_text()
    assert 'ERROR:' not in text, text
    captures = {Path(step['path']).stem: row['snapshot']['map'] for step, row in zip(steps, result['steps'])
                if step['action'] == 'capture'}
    before, panned, returned = (captures[name] for name in ('before', 'panned', 'returned'))
    assert panned['camera_x'] - before['camera_x'] == 80 and panned['camera_y'] - before['camera_y'] == -40, captures
    assert returned['camera_x'] == before['camera_x'] and returned['camera_y'] == before['camera_y'], captures
    assert (output / 'panned.png').read_bytes() != (output / 'before.png').read_bytes(), 'Pan must change actual pixels'
    assert (output / 'returned.png').read_bytes() == (output / 'before.png').read_bytes(), 'Pan round trip must restore pixels'
    (output / 'camera-results.json').write_text(json.dumps({'passed': True, 'scope': __doc__,
        'checks': ['GUI mouse motion/button route', 'camera displacement', 'GPU pixels change',
                   'paused UI blocks mouse', 'reverse drag restores pixels', 'normal map close'],
        'captures': captures}, indent=2) + '\n')
    print('PASS: graphical mouse navigation and GPU pixel round trip')


if __name__ == '__main__': main()
