#!/usr/bin/env python3
"""Exercise the in-game console against ALL01's real selected unit."""
import argparse
import json
import subprocess
import tempfile
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--godot", default="godot")
    parser.add_argument("--game-data", type=Path, required=True)
    args = parser.parse_args()
    project = Path(__file__).resolve().parents[1]
    with tempfile.TemporaryDirectory(prefix="ra2-console-") as temporary:
        work = Path(temporary)
        steps = [
            {"action": "wait_boot"},
            {"action": "set_map", "map": "ALL01UMD.MAP"},
            {"action": "press", "node": "MainMenu/Buttons/StartButton"},
            {"action": "wait_map", "state": "drawn", "timeout_seconds": 90},
            {"action": "key", "key": "Quoteleft"},
            {"action": "assert", "field": "console_visible", "equals": True},
            {"action": "console_command", "text": "power"},
            {"action": "assert", "field": "console_output", "equals": "已为当前玩家增加 1000000 电力（本局持续生效）。"},
            {"action": "key", "key": "Quoteleft"},
            {"action": "select_unit"},
            {"action": "key", "key": "Quoteleft"},
            {"action": "console_command", "text": "level 2"},
            {"action": "assert_selected", "level": 2},
            {"action": "console_command", "text": "life 5000"},
            {"action": "assert_selected", "level": 2, "health": 5000},
            {"action": "key", "key": "Up"},
            {"action": "assert", "field": "console_input", "equals": "life 5000"},
            {"action": "key", "key": "Up"},
            {"action": "assert", "field": "console_input", "equals": "level 2"},
            {"action": "key", "key": "Down"},
            {"action": "assert", "field": "console_input", "equals": "life 5000"},
            {"action": "key", "key": "Quoteleft"},
            {"action": "assert", "field": "console_visible", "equals": False},
        ]
        script = work / "steps.json"
        report = work / "report.json"
        script.write_text(json.dumps({"steps": steps}, ensure_ascii=False))
        with (work / "godot.log").open("w") as log:
            result = subprocess.run([
                args.godot, "--path", str(project), "--rendering-method", "mobile", "--",
                "--game-data=" + str(args.game_data.resolve()),
                "--display-config=" + str(work / "display.cfg"),
                "--resource-config=" + str(work / "resources.cfg"),
                "--automation=" + str(script), "--automation-report=" + str(report),
            ], stdout=log, stderr=subprocess.STDOUT, timeout=150)
        outcome = json.loads(report.read_text()) if report.exists() else {}
        if result.returncode != 0 or not outcome.get("passed"):
            raise AssertionError(outcome.get("error") or (work / "godot.log").read_text())
        history = outcome["snapshot"]["console_history"]
        for command in ("> power", "> level 2", "> life 5000"):
            if command not in history:
                raise AssertionError("Missing console history entry: " + command)
    print("PASS: Console commands")


if __name__ == "__main__":
    main()
