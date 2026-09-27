#!/usr/bin/env python3
"""Drive actual radar GUI clicks/drags through Godot's opt-in runtime automation."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--godot', default='godot')
    p.add_argument('--app', type=Path)
    p.add_argument('--game-data', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    project = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    command = [args.godot, '--path', str(project), '--rendering-method', 'mobile']
    if args.app:
        app = args.app.resolve()
        command = [str(app / 'Contents/MacOS/ra2opengodot' if app.suffix == '.app' else app)]
    def move(x, y, width=1280):
        # Fixed original layout fixture; production has no radar Control.
        return {'action': 'mouse_move', 'node': 'GameScreen/WorldSurface',
                'x': width-152+x*140, 'y': 49+y*108}
    def button(pressed, name='Left'):
        return {'action': 'mouse_button', 'button': name, 'pressed': pressed}
    def capture(name):
        return {'action': 'capture', 'path': str(output / (name + '.png')), 'min_colors': 32}
    def assertion(field, value):
        return {'action': 'assert', 'field': field, 'equals': value}
    wait = {'action': 'wait_map', 'state': 'drawn'}
    escape = {'action': 'key', 'key': 'Escape'}
    menu = {'action': 'press', 'node': 'PauseMenu/Panel/Buttons/MainMenuButton'}
    start = {'action': 'press', 'node': 'MainMenu/Buttons/StartButton'}
    steps = [{'action': 'wait_boot'}, {'action': 'set_map', 'map': 'ALL01UMD.MAP'}, start, wait, assertion('map.radar_available', True),
        assertion('map.radar_frame_valid', True), capture('before'), move(.5, .65), button(True), wait,
        capture('clicked'), move(.58, .8), wait, capture('dragged'), button(False),
        move(.98, .5), button(True), button(False), wait, capture('letterbox-ignored'),
        # Original right press also navigates; it must use radar coordinates.
        move(.6, .7), button(True, 'Right'), move(.65, .75), button(False, 'Right'), wait, capture('right-navigated'),
        escape, assertion('paused', True), move(.3, .3), button(True), move(.7, .3), button(False),
        {'action': 'frames', 'count': 8}, capture('paused'), escape, wait, capture('resumed'),
        {'action': 'resize', 'width': 1600, 'height': 900},
        {'action': 'wait_map', 'state': 'drawn', 'width': 1600, 'height': 900},
        move(.4, .5, 1600), button(True), button(False), wait, capture('resized'), escape, menu,
        {'action': 'set_map', 'map': 'SOV02SMD.MAP'}, start, wait,
        assertion('map.radar_available', True), assertion('map.radar_frame_valid', True), capture('snow'),
        escape, menu, assertion('map.state', 'empty')]
    script, report = output / 'radar.json', output / 'radar-report.json'
    script.write_text(json.dumps({'steps': steps}, indent=2) + '\n')
    report.unlink(missing_ok=True)
    env = os.environ.copy()
    env.pop('RA2_GAME_DATA', None)
    with tempfile.TemporaryDirectory(prefix='ra2-radar-') as temporary:
        with (output / 'radar.log').open('w') as log:
            process = subprocess.run(command + ['--', '--game-data=' + str(args.game_data.resolve()),
                '--resource-config=' + str(Path(temporary) / 'resources.cfg'),
                '--display-config=' + str(Path(temporary) / 'display.cfg'),
                '--automation=' + str(script), '--automation-report=' + str(report)], cwd=temporary,
                env=env, stdout=log, stderr=subprocess.STDOUT, timeout=150)
    result = json.loads(report.read_text()) if report.exists() else {}
    assert process.returncode == 0 and result.get('passed'), f'See {report} and {output / "radar.log"}'
    log = (output / 'radar.log').read_text()
    assert 'ERROR:' not in log, log
    captures = {Path(step['path']).stem: row['snapshot']['map'] for step, row in zip(steps, result['steps'])
                if step['action'] == 'capture'}
    def camera(name):
        return captures[name]['camera_x'], captures[name]['camera_y']
    assert camera('before') != camera('clicked') != camera('dragged'), captures
    assert camera('right-navigated') != camera('letterbox-ignored'), captures
    assert camera('dragged') == camera('letterbox-ignored') and camera('right-navigated') == camera('paused') == camera('resumed'), captures
    assert (output / 'clicked.png').read_bytes() != (output / 'before.png').read_bytes(), 'Click must change rendered pixels'
    assert (output / 'dragged.png').read_bytes() != (output / 'clicked.png').read_bytes(), 'Drag must change rendered pixels'
    assert captures['snow']['theater'] == 1 and captures['snow']['radar_available'], captures['snow']
    assert captures['resized']['viewport_width'] == 1600 and captures['resized']['radar_frame_valid'], captures['resized']
    (output / 'radar-results.json').write_text(json.dumps({'passed': True, 'scope': __doc__,
        'checks': ['original terrain radar available', 'height-aware viewport frame', 'actual GUI left click and drag',
                   'radar blocks world right-drag', 'pause blocks radar input', 'resize navigation', 'snow reload', 'normal close'],
        'captures': captures}, indent=2) + '\n')
    print('PASS: radar GUI click/drag, viewport frame, input isolation, resize and reload')


if __name__ == '__main__':
    main()
