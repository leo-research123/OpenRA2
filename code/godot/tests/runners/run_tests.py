"""Exercise native resource boot and UI with synthetic packs; using only authored inputs."""
import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile
import struct
import zlib
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "support"))
from synthetic_fixtures import mix, csf, text_ini_fixtures, skirmish_map

parser = argparse.ArgumentParser()
parser.add_argument("--godot", default="godot", help="Path to the Godot executable")
parser.add_argument("--dotnet", default="dotnet", help="Path to the .NET SDK executable")
parser.add_argument("--windowed", action="store_true", help="Also exercise actual desktop windows")
parser.add_argument("--capture-dir", type=Path, help="Optional window screenshots")
args = parser.parse_args()
project = Path(__file__).resolve().parents[2]

subprocess.run(
    [args.dotnet, "build", str(project / "OpenRA2.csproj"), "--nologo"],
    check=True, timeout=120,
)
subprocess.run(
    [args.godot, "--headless", "--editor", "--import", "--path", str(project)],
    check=True, timeout=90,
)


def run(path, *extra, scene="TestApp", windowed=False):
    result = subprocess.run(
        [args.godot, *([] if windowed else ["--headless"]), "--path", str(path),
         f"res://tests/{'windowed' if windowed else 'headless'}/{scene}.tscn", *extra],
        timeout=90, capture_output=True, text=True, encoding="utf-8",
    )
    print(result.stdout, end="")
    print(result.stderr, end="")
    result.check_returncode()
    if f"PASS: {scene}" not in result.stdout and not (windowed and scene == "TestDisplayWindow" and result.returncode == 0):
        raise RuntimeError("The C# application test did not complete")
    if scene == "TestAudioShutdown" and any(marker in result.stdout + result.stderr for marker in
        ("ERROR:", "SCRIPT ERROR:", "ObjectDB instances leaked", "Leaked instance:", "resources still in use", "SIGSEGV", "SIGABRT")):
        raise RuntimeError("Audio lifecycle exit reported an engine error or leaked object")
    if scene in ("TestINI", "TestMapSelection") and "Unicode parsing error" in result.stdout + result.stderr:
        raise RuntimeError("Native INI text must not produce Unicode parsing errors")


with tempfile.TemporaryDirectory(prefix="ra2-bootstrap-") as directory:
    data = Path(directory)
    for name in ("langmd.mix", "language.mix", "expandmd99.mix"):
        (data / name).write_bytes(mix({}))
    for suffix in ("md", ""):
        (data / f"ra2{suffix}.mix").write_bytes(mix({
            f"cache{suffix}.mix": mix({"test.txt": b"synthetic"}),
            f"local{suffix}.mix": mix({"config.ini": b"[General]\nName=primary\nCount=42\nEnabled=yes\nRatio=12.5%\n[Other]\nName=secondary\n", **text_ini_fixtures()}),
        }))
    (data / "thememd.mix").write_bytes(b"CLASS")
    run(project, "--", f"--game-data={data}")
    for mode in ("loading", "direct", "close-first", "repeat", "reopen"):
        run(project, "--", f"--game-data={data}", f"--shutdown-mode={mode}", scene="TestAudioShutdown")
    worker = subprocess.run(
        [args.godot, "--headless", "--path", str(project),
         "res://tests/headless/TestResourceWorker.tscn", "--", f"--game-data={data}"],
        timeout=45, capture_output=True, text=True, encoding="utf-8",
    )
    print(worker.stdout, end="")
    print(worker.stderr, end="")
    worker.check_returncode()
    if "PASS: TestResourceWorker" not in worker.stdout:
        raise RuntimeError("The native resource worker test did not complete")
    ini_test = subprocess.run(
        [args.godot, "--headless", "--path", str(project),
         "res://tests/headless/TestINI.tscn", "--", f"--game-data={data}"],
        timeout=45, capture_output=True, text=True, encoding="utf-8",
    )
    print(ini_test.stdout, end="")
    print(ini_test.stderr, end="")
    ini_test.check_returncode()
    if "PASS: TestINI" not in ini_test.stdout:
        raise RuntimeError("The INI bridge test did not complete")
    run(project, "--", f"--game-data={data}", "--expect-no-maps", scene="TestMapSelection")
    header = b"[Map]\nSize=0,0,8,12\nTheater=TEMPERATE\n[Basic]\nName=Packed map\n"
    (data / "ra2md.csf").write_bytes(csf({"Name:All01md": "尤里盟军首关", "Name:Sov01md": "尤里苏军首关"}))
    (data / "mapsmd01.mix").write_bytes(mix({
        "pack.map": header, "mission.map": header + b"MultiplayerOnly=no\n",
        "extra.map": header + b"MultiplayerOnly=yes\n",
        "battlemd.ini": b"[Battles]\n1=First\n2=Duplicate\n3=Missing\n[First]\nScenario=PACK.MAP\n[Duplicate]\nScenario=pack.map\n[Missing]\nScenario=missing.map\n[MixedBase]\nScenario=ALL01T.MAP\n[MixedCustom]\nScenario=ALL99T.MAP\n",
        "missionmd.ini": b"[MISSION.MAP]\nUIName=Name:Mission\n[missing.map]\nUIName=Name:Missing\n[ALL01UMD.MAP]\nUIName=Name:All01md\n",
        "ALL01UMD.MAP": header.replace(b"Name=Packed map", b"Name="),
        # Built-in candidates must find packed stock maps omitted by all installation indexes.
        "SOV01UMD.MAP": header.replace(b"Name=Packed map", b"Name="),
    }))
    (data / "battle.ini").write_bytes(b"[Battles]\n1=Base\n2=Shared\n[Base]\nScenario=misnamedmd.map\n[Shared]\nScenario=pack.map\n")
    (data / "mission.ini").write_bytes(b"[ALL01T.MAP]\n[SoV01t.MaP]\n[TRN01T.MAP]\n")
    for name in ("ALL01T.MAP", "sov01t.map", "TRN01T.MAP", "alliance-md.map", "misnamedmd.map",
                 "ALL99T.MAP", "ALL01S.MAP", "SOV08TMD.MAP"):
        (data / name).write_bytes(header)
    (data / "multimd.mix").write_bytes(mix({
        "missionsmd.pkt": b"[MultiMaps]\n1=ARENA\n2=missing\n[ARENA]\nDescription=An arena\n",
        "arena.map": header + b"MultiplayerOnly=true\n",
    }))
    (data / "pack.map").write_bytes(header.replace(b"Packed map", b"Loose override"))
    (data / "user.yrm").write_bytes(header.replace(b"Packed map", "UTF-8 中文联机地图".encode("utf-8")) + b"MultiplayerOnly=yes\n")
    (data / "custom.mpr").write_bytes(header.replace(b"Packed map", "GBK 中文自定义任务".encode("gbk")) + b"MultiplayerOnly=false\n")
    (data / "extra.pkt").write_bytes(b"[MultiMaps]\n1=EXTRA\n")
    (data / "invalid.map").write_bytes(b"[Basic]\nName=Not a map\n")
    (data / "bad-size.map").write_bytes(b"[Map]\nSize=0,0,-1,broken\n")
    (data / "catalog-empty").mkdir()
    (data / "catalog-empty/ra2md.mix").write_bytes(mix({}))
    run(project, "--", f"--game-data={data}", scene="TestMapSelection")
    run(project, "--", f"--game-data={data}", "--map=missing.map", scene="TestMapSelection")
    # A separate authored catalog provides real preview bytes and two starts.
    lobby_data = data / "lobby"
    lobby_data.mkdir()
    for name in ("langmd.mix", "language.mix", "ra2md.mix"):
        (lobby_data / name).write_bytes(mix({}))
    (lobby_data / "arena.map").write_bytes(skirmish_map())
    (lobby_data / "rulesmd.ini").write_bytes(
        b"[Countries]\n0=TestAlpha\n1=TestBeta\n[TestAlpha]\nMultiplay=yes\n"
        b"[TestBeta]\nMultiplay=yes\n")
    run(project, "--", f"--game-data={lobby_data}", scene="TestSkirmishLobby")
    if args.windowed:
        captures = []
        if args.capture_dir:
            args.capture_dir.mkdir(parents=True, exist_ok=True)
            captures = [f"--capture-dir={args.capture_dir.resolve()}"]
        for scene in ("TestResourceWindow", "TestDisplayWindow"):
            run(project, "--", f"--game-data={data}", *captures, scene=scene, windowed=True)
    (data / "mapsmd01.mix").write_bytes(b"BROKEN")
    run(project, "--", f"--game-data={data}", "--expect-catalog-failure", scene="TestMapSelection")
    (data / "ra2md.mix").write_bytes(b"CLASS")
    run(project, "--", f"--game-data={data}", "--expect-resource-failure")
    run(project, "--", f"--game-data={data / 'missing'}", "--expect-resource-failure")
    empty = data / "empty"
    empty.mkdir()
    run(project, "--", f"--game-data={empty}", "--expect-empty")
with tempfile.TemporaryDirectory(prefix="ra2-missing-core-") as directory:
    fixture = Path(directory)
    (fixture / "project.godot").write_text(
        'config_version=5\n[application]\nconfig/name="OpenRA2"\n'
        'config/features=PackedStringArray("4.7", "C#")\n'
        '[dotnet]\nproject/assembly_name="OpenRA2"\n',
        encoding="utf-8",
    )
    for folder in ("scenes", "scripts", "ui", "tests"):
        shutil.copytree(project / folder, fixture / folder)
    # Run the same C# assembly with no GDExtension file or native library present.
    shutil.copytree(
        project / ".godot/mono/temp/bin/Debug",
        fixture / ".godot/mono/temp/bin/Debug",
    )
    run(fixture, "--", "--expect-missing-core")
print("PASS: resource progress, map discovery/selection, CLI map entry, navigation, authored lobby, resource/core errors")
