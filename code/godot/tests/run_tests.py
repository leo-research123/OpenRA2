"""Exercise native resource boot and UI with synthetic packs; original assets are optional."""
import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile
import struct
import zlib

parser = argparse.ArgumentParser()
parser.add_argument("--godot", default="godot", help="Path to the Godot executable")
parser.add_argument("--dotnet", default="dotnet", help="Path to the .NET SDK executable")
parser.add_argument("--game-data", type=Path, help="Also exercise a real installation directory")
args = parser.parse_args()
project = Path(__file__).resolve().parents[1]

subprocess.run(
    [args.dotnet, "build", str(project / "ra2opengodot.csproj"), "--nologo"],
    check=True, timeout=120,
)
subprocess.run(
    [args.godot, "--headless", "--editor", "--import", "--path", str(project)],
    check=True, timeout=90,
)


def run(path, *extra, scene="TestApp"):
    result = subprocess.run(
        [args.godot, "--headless", "--path", str(path),
         f"res://tests/{scene}.tscn", *extra],
        timeout=45, capture_output=True, text=True, encoding="utf-8",
    )
    print(result.stdout, end="")
    print(result.stderr, end="")
    result.check_returncode()
    if f"PASS: {scene}" not in result.stdout:
        raise RuntimeError("The C# application test did not complete")


def mix(files):
    rows = []
    for name, data in files.items():
        name = name.upper().encode("ascii")
        remainder = len(name) % 4
        if remainder:
            name += bytes([remainder]) + name[len(name) - remainder:][:1] * (3 - remainder)
        identifier = zlib.crc32(name)
        rows.append((identifier, data))
    rows.sort(key=lambda row: struct.unpack("<i", struct.pack("<I", row[0]))[0])
    entries, body = b"", b""
    for identifier, data in rows:
        entries += struct.pack("<III", identifier, len(body), len(data))
        body += data
    return struct.pack("<IHI", 0, len(rows), len(body)) + entries + body


with tempfile.TemporaryDirectory(prefix="ra2-bootstrap-") as directory:
    data = Path(directory)
    for name in ("langmd.mix", "language.mix", "expandmd99.mix"):
        (data / name).write_bytes(mix({}))
    for suffix in ("md", ""):
        (data / f"ra2{suffix}.mix").write_bytes(mix({
            f"cache{suffix}.mix": mix({"test.txt": b"synthetic"}),
            f"local{suffix}.mix": mix({"config.ini": b"[General]\nName=primary\nCount=42\nEnabled=yes\nRatio=12.5%\n[Other]\nName=secondary\n"}),
        }))
    (data / "thememd.mix").write_bytes(b"CLASS")
    run(project, "--", f"--game-data={data}")
    worker = subprocess.run(
        [args.godot, "--headless", "--path", str(project),
         "res://tests/TestResourceWorker.tscn", "--", f"--game-data={data}"],
        timeout=45, capture_output=True, text=True, encoding="utf-8",
    )
    print(worker.stdout, end="")
    print(worker.stderr, end="")
    worker.check_returncode()
    if "PASS: TestResourceWorker" not in worker.stdout:
        raise RuntimeError("The native resource worker test did not complete")
    ini_test = subprocess.run(
        [args.godot, "--headless", "--path", str(project),
         "res://tests/TestINI.tscn", "--", f"--game-data={data}"],
        timeout=45, capture_output=True, text=True, encoding="utf-8",
    )
    print(ini_test.stdout, end="")
    print(ini_test.stderr, end="")
    ini_test.check_returncode()
    if "PASS: TestINI" not in ini_test.stdout:
        raise RuntimeError("The INI bridge test did not complete")
    run(project, "--", f"--game-data={data}", "--expect-no-maps", scene="TestMapSelection")
    header = b"[Map]\nSize=0,0,8,12\nTheater=TEMPERATE\n[Basic]\nName=Packed map\n"
    (data / "mapsmd01.mix").write_bytes(mix({
        "pack.map": header, "mission.map": header + b"MultiplayerOnly=no\n",
        "extra.map": header + b"MultiplayerOnly=yes\n",
        "battlemd.ini": b"[Battles]\n1=First\n2=Duplicate\n3=Missing\n[First]\nScenario=PACK.MAP\n[Duplicate]\nScenario=pack.map\n[Missing]\nScenario=missing.map\n",
        "missionmd.ini": b"[MISSION.MAP]\nUIName=Name:Mission\n[missing.map]\nUIName=Name:Missing\n",
    }))
    (data / "multimd.mix").write_bytes(mix({
        "missionsmd.pkt": b"[MultiMaps]\n1=ARENA\n2=missing\n[ARENA]\nDescription=An arena\n",
        "arena.map": header + b"MultiplayerOnly=true\n",
    }))
    (data / "pack.map").write_bytes(header.replace(b"Packed map", b"Loose override"))
    (data / "user.yrm").write_bytes(header + b"MultiplayerOnly=yes\n")
    (data / "custom.mpr").write_bytes(header + b"MultiplayerOnly=false\n")
    (data / "extra.pkt").write_bytes(b"[MultiMaps]\n1=EXTRA\n")
    (data / "invalid.map").write_bytes(b"[Basic]\nName=Not a map\n")
    (data / "bad-size.map").write_bytes(b"[Map]\nSize=0,0,-1,broken\n")
    (data / "catalog-empty").mkdir()
    (data / "catalog-empty/ra2md.mix").write_bytes(mix({}))
    run(project, "--", f"--game-data={data}", scene="TestMapSelection")
    run(project, "--", f"--game-data={data}", "--map=missing.map", scene="TestMapSelection")
    (data / "mapsmd01.mix").write_bytes(b"BROKEN")
    run(project, "--", f"--game-data={data}", "--expect-catalog-failure", scene="TestMapSelection")
    (data / "ra2md.mix").write_bytes(b"CLASS")
    run(project, "--", f"--game-data={data}", "--expect-resource-failure")
    run(project, "--", f"--game-data={data / 'missing'}", "--expect-resource-failure")
    empty = data / "empty"
    empty.mkdir()
    run(project, "--", f"--game-data={empty}", "--expect-empty")
if args.game_data:
    run(project, "--", f"--game-data={args.game_data.resolve()}")
with tempfile.TemporaryDirectory(prefix="ra2-missing-core-") as directory:
    fixture = Path(directory)
    (fixture / "project.godot").write_text(
        'config_version=5\n[application]\nconfig/name="ra2opengodot"\n'
        'config/features=PackedStringArray("4.7", "C#")\n'
        '[dotnet]\nproject/assembly_name="ra2opengodot"\n',
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
print("PASS: resource progress, map discovery/selection, CLI map entry, navigation, pause/resume, resource/core errors")
