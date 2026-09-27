"""Shared compile/link mechanics; each probe keeps its sources and ABI options."""
import json
import shlex
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]


def build_probe(database, directory, sources, filename, *, object_suffix='.obj',
                flags=('/GS-', '/Gy'), symbols=False, commands_file=None,
                require_x86=False):
    entry = next(c for c in json.loads(database.read_text())
                 if c['file'].endswith('/RadarClassState.cpp'))
    command = shlex.split(entry['command'])
    command = [a for a in command[:command.index('-c')]
               if not a.startswith(('/Fo', '/Fd'))]
    if require_x86:
        assert any('i686-pc-windows-msvc' in a for a in command), 'x86 MSVC compile command required'
    commands, objects = [], []
    for source in sources:
        obj = directory / (Path(source).stem + object_suffix)
        call = command + list(flags) + ['/Fo' + str(obj), '-c', '--', str(ROOT / source)]
        subprocess.run(call, check=True)
        commands.append(call)
        objects.append(str(obj))
    linker = shutil.which('lld-link')
    assert linker, 'lld-link is required'
    dll = directory / filename
    call = [linker, '/dll', '/noentry', '/nodefaultlib', '/machine:x86',
            '/base:0x20000000', '/dynamicbase:no', '/opt:ref', '/out:' + str(dll)]
    if symbols:
        call.append('/map:' + str(dll.with_suffix('.map')))
    call += objects
    subprocess.run(call, check=True)
    commands.append(call)
    if commands_file:
        (directory / commands_file).write_text(json.dumps(commands, indent=2) + '\n')
    return (dll, dll.with_suffix('.map')) if symbols else dll
