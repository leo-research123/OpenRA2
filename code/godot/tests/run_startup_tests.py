#!/usr/bin/env python3
"""Drive the actual game (editor executable or release export) without desktop automation.

Checks missing resources, directory selection, persisted startup without --game-data,
command-line precedence, map failure/recovery, pause, switching maps and GPU resize.
Each case writes its input JSON, process log, status report and screenshots.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--godot', default='godot')
    parser.add_argument('--app', type=Path, help='Exported executable, or a macOS .app bundle')
    parser.add_argument('--game-data', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    project = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    game_data = str(args.game_data.resolve())
    command = [args.godot, '--path', str(project), '--rendering-method', 'mobile']
    if args.app:
        app = args.app.resolve()
        command = [str(app / 'Contents/MacOS/ra2opengodot' if app.suffix == '.app' else app)]
    environment = os.environ.copy()
    environment.pop('RA2_GAME_DATA', None)
    results = []

    def run(name, steps, extra=()):
        scenario = output / (name + '.json')
        report = output / (name + '-report.json')
        scenario.write_text(json.dumps({'steps': steps}, indent=2) + '\n')
        report.unlink(missing_ok=True)
        with (output / (name + '.log')).open('w') as log:
            process = subprocess.run(command + ['--', '--automation=' + str(scenario),
                '--automation-report=' + str(report), '--display-config=' + str(display),
                '--resource-config=' + str(resources), *extra],
                cwd=temporary, env=environment, stdout=log, stderr=subprocess.STDOUT, timeout=150)
        result = json.loads(report.read_text()) if report.exists() else {}
        text = (output / (name + '.log')).read_text()
        passed = process.returncode == 0 and result.get('passed') is True and 'PASS: AutomationRunner' in text
        # Resource failures are intentionally exercised; unexpected engine/GPU errors are not.
        unexpected = [line for line in text.splitlines() if line.startswith('ERROR:') and
                      not line.startswith(('ERROR: 未找到游戏资源', 'ERROR: 资源加载失败', 'ERROR: 地图加载失败'))]
        passed = passed and not unexpected
        results.append({'case': name, 'passed': passed, 'exit_code': process.returncode,
                        'unexpected_errors': unexpected})
        print(name, 'PASS' if passed else 'FAIL', flush=True)
        (output / 'startup-results.json').write_text(json.dumps(results, indent=2) + '\n')
        if not passed:
            raise RuntimeError(f'{name} failed; see {report} and {output / (name + ".log")}')
        return result

    def press(node): return {'action': 'press', 'node': node}
    def assertion(field, value): return {'action': 'assert', 'field': field, 'equals': value}
    def capture(name): return {'action': 'capture', 'node': 'GameScreen/WorldViewport',
                              'path': str(output / (name + '.png')), 'min_colors': 32}
    start = press('MainMenu/Buttons/StartButton')
    def choose(filename):
        return [start, assertion('screen', 'MapSelection'), {'action': 'wait_map_list'},
                {'action': 'select_map', 'map': filename},
                press('MapSelectionScreen/Content/Actions/LoadButton')]
    drawn = {'action': 'wait_map', 'state': 'drawn'}
    escape = {'action': 'key', 'key': 'Escape'}
    menu = press('PauseMenu/Panel/Buttons/MainMenuButton')
    with tempfile.TemporaryDirectory(prefix='ra2-startup-') as temporary:
        resources = Path(temporary) / 'resources.cfg'
        display = Path(temporary) / 'display.cfg'
        # In a release bundle this exercises the actual no-argument fallback.
        # Development Godot has an intentional repo fallback; override it with an empty directory.
        first_args = [] if args.app else ['--game-data=' + temporary]
        run('first-start-and-repair', [
            {'action': 'wait_boot_error'},
            assertion('resources.state', 'complete'), assertion('resources.mounted', []),
            press('ResourceDirectoryButton'), {'action': 'choose_resources', 'directory': game_data},
            {'action': 'wait_boot'}, assertion('resource_directory', game_data), *choose('ALL01UMD.MAP'), drawn,
            capture('first-start-map'), escape, assertion('paused', True), menu,
            assertion('map.state', 'empty')], first_args)
        assert resources.exists(), 'Successful directory selection must persist'
        run('remembered-directory', [
            {'action': 'wait_boot'}, assertion('resource_directory', game_data), *choose('ALL01UMD.MAP'), drawn,
            {'action': 'resize', 'width': 1600, 'height': 900},
            {'action': 'wait_map', 'state': 'drawn', 'width': 1600, 'height': 900},
            capture('resized-map'), escape, menu,
            *choose('SOV02SMD.MAP'), drawn, capture('snow-map'), escape, menu])
        run('command-line-map', [
            {'action': 'wait_boot'}, start, assertion('screen', 'Game'),
            assertion('map_file', 'SOV02SMD.MAP'), assertion('map_list_count', 0), drawn,
            capture('command-line-map'), escape, menu], ['--map=SOV02SMD.MAP'])
        run('picker-keyboard', [
            {'action': 'wait_boot'}, start, {'action': 'wait_map_list'},
            assertion('map_category', 'Campaign'),
            {'action': 'select_map', 'map': 'SOV02SMD.MAP'},
            {'action': 'filter_maps', 'category': 'NonCampaign'}, assertion('selected_map', ''),
            {'action': 'capture', 'path': str(output / 'map-picker-non-campaign.png')},
            {'action': 'filter_maps', 'category': 'All'},
            {'action': 'select_map', 'map': 'SOV02SMD.MAP'},
            {'action': 'filter_maps', 'category': 'Campaign'}, assertion('selected_map', ''),
            {'action': 'select_map', 'map': 'SOV02SMD.MAP'},
            {'action': 'capture', 'path': str(output / 'map-picker.png')},
            {'action': 'key', 'key': 'Enter'}, assertion('screen', 'Game'), drawn, escape, menu,
            {'action': 'resize', 'width': 960, 'height': 540}, start, {'action': 'wait_map_list'},
            assertion('map_category', 'Campaign'),
            {'action': 'capture', 'path': str(output / 'map-picker-small.png')},
            escape, assertion('screen', 'Main'), assertion('map_list_scanning', False)])
        remembered = resources.read_bytes()
        run('invalid-explicit-directory', [{'action': 'wait_boot_error'}],
            ['--game-data=' + str(Path(temporary) / 'missing')])
        assert resources.read_bytes() == remembered, 'Failed directory must not replace the saved one'
        run('map-error-and-recovery', [
            {'action': 'wait_boot'}, {'action': 'set_map', 'map': 'missing-map.map'}, start,
            {'action': 'wait_map', 'state': 'failed'},
            assertion('map.error', 'Map file not found in the resource directory or mounted MIX packages'),
            {'action': 'frames', 'count': 2}, escape, menu, assertion('screen', 'Main'),
            {'action': 'set_map', 'map': 'ALL01UMD.MAP'}, start, drawn, capture('recovered-map')])
    print('PASS: startup automation (no Computer Use)', flush=True)


if __name__ == '__main__':
    main()
