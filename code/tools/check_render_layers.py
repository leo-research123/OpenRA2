#!/usr/bin/env python3
"""Render layering guard.

Counts and lists layer violations. Without --enforce it never fails: the exit status is
0 unless the script itself cannot run (bad arguments, missing directories).
Run it from anywhere:

    python3 code/tools/check_render_layers.py [--root <OpenRA2>] [--json]

Rules (see docs/plans/render-layering-2026-10-21.md in the research repository):

  (a) code/core/include/yrpp must not use the project's `game::` namespace.
      (a2) It must also not declare `namespace game` (forward declarations) or
      include project headers from `api/`.
  (b) code/core/src/yrpp must not include the drawing ABI headers under
      core/include/api, nor any bridge header. Informational line (b2) also
      lists the project-internal drawing headers in core/src that yrpp files
      include; those are the next layer boundary and are not counted in (b).
  (c) code/bridge/src must not include yrpp headers.
  (d) code/core/src/presentation must not cast away const of a game object (`const_cast` to anything but a
      character pointer): presentation reads the simulation only.
  (e) code/core/src/presentation may use render/frame.hpp; backend, runtime and Godot headers are forbidden.
  (f) code/core/src/yrpp must not include presentation headers (the simulation does not depend on presentation).

With --enforce the script exits non-zero when a rule has more hits than registered in
code/tools/render_layers_exceptions.json. The file lists the temporary exceptions of the migration with their counts;
a count may only go down, and a phase may only end with the exceptions of the rules it was not asked to clear.
"""
import argparse
import json
import pathlib
import re
import sys

# Drawing ABI headers in core/include/api.
DRAWING_API_HEADERS = {
    "type_drawing.hpp",
    "software_type_drawing.hpp",
    "voxel_drawing.hpp",
    "drawing_resources.hpp",
    "drawing_diagnostics.hpp",
}

# Project-internal drawing headers in core/src (informational only).
INTERNAL_DRAWING_HEADERS = {
    "type_drawing.hpp",
    "type_drawing_software.hpp",
    "tactical_drawing.hpp",
    "voxel_drawing.hpp",
    "sprite_drawing.hpp",
    "building_drawing.hpp",
    "techno_drawing.hpp",
    "techno_extras_drawing.hpp",
    "overlay_drawing.hpp",
    "bitfont_drawing.hpp",
    "background_redraw.hpp",
    "terrain_sprite_layer.hpp",
}

SOURCE_SUFFIXES = {".h", ".hpp", ".inc", ".c", ".cc", ".cpp", ".cxx"}
INCLUDE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]')
GAME_NS = re.compile(r"(?<![A-Za-z0-9_])game\s*::")
GAME_DECL = re.compile(r"\bnamespace\s+game\b")


def source_files(directory):
    if not directory.is_dir():
        raise SystemExit(f"missing directory: {directory}")
    return sorted(p for p in directory.rglob("*") if p.is_file() and p.suffix in SOURCE_SUFFIXES)


def strip_comments(lines):
    """Yield (line number, text) with // and /* */ comments blanked."""
    in_block = False
    for number, line in enumerate(lines, 1):
        out = []
        i = 0
        while i < len(line):
            if in_block:
                end = line.find("*/", i)
                if end < 0:
                    i = len(line)
                else:
                    in_block = False
                    i = end + 2
                continue
            if line.startswith("/*", i):
                in_block = True
                i += 2
                continue
            if line.startswith("//", i):
                break
            out.append(line[i])
            i += 1
        yield number, "".join(out)


CONST_CAST = re.compile(r"\bconst_cast\s*<\s*([^>]+?)\s*>")
CHAR_POINTER = re.compile(r"^(?:const\s+)?(?:w?char(?:_t)?|unsigned\s+char|std::byte)\s*\*$")
FORBIDDEN_PRESENTATION = ("bridge/", "render/", "runtime/", "godot", "godot_cpp/")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path(__file__).resolve().parents[2])
    parser.add_argument("--enforce", action="store_true", help="fail when a rule exceeds its registered exceptions")
    parser.add_argument("--json", action="store_true", help="print machine-readable counts and locations")
    args = parser.parse_args()
    code = args.root / "code"

    a_hits, a2_hits, b_hits, b2_hits, c_hits = [], [], [], [], []
    d_hits, e_hits, f_hits = [], [], []

    for path in source_files(code / "core/include/yrpp"):
        for number, text in strip_comments(path.read_text(errors="replace").splitlines()):
            if GAME_NS.search(text):
                a_hits.append((path, number, text.strip()))
            elif GAME_DECL.search(text):
                a2_hits.append((path, number, text.strip()))
            else:
                include = INCLUDE.match(text)
                if include and include.group(1).startswith("api/"):
                    a2_hits.append((path, number, text.strip()))

    for path in source_files(code / "core/src/yrpp"):
        for number, text in strip_comments(path.read_text(errors="replace").splitlines()):
            match = INCLUDE.match(text)
            if not match:
                continue
            target = match.group(1)
            if target.startswith("api/") and pathlib.PurePosixPath(target).name in DRAWING_API_HEADERS:
                b_hits.append((path, number, target))
            elif target.startswith("bridge/"):
                b_hits.append((path, number, target))
            elif target in INTERNAL_DRAWING_HEADERS:
                b2_hits.append((path, number, target))

    for path in source_files(code / "bridge/src"):
        for number, text in strip_comments(path.read_text(errors="replace").splitlines()):
            match = INCLUDE.match(text)
            if match and match.group(1).startswith("yrpp/"):
                c_hits.append((path, number, match.group(1)))

    presentation = code / "core/src/presentation"
    if presentation.is_dir():
        for path in source_files(presentation):
            for number, text in strip_comments(path.read_text(errors="replace").splitlines()):
                for match in CONST_CAST.finditer(text):
                    if not CHAR_POINTER.match(match.group(1).strip()):
                        d_hits.append((path, number, text.strip()))
                include = INCLUDE.match(text)
                if (include and include.group(1) != "render/frame.hpp" and
                        include.group(1).lower().startswith(FORBIDDEN_PRESENTATION)):
                    e_hits.append((path, number, include.group(1)))
    for path in source_files(code / "core/src/yrpp"):
        for number, text in strip_comments(path.read_text(errors="replace").splitlines()):
            include = INCLUDE.match(text)
            if include and include.group(1).startswith("presentation/"):
                f_hits.append((path, number, include.group(1)))

    def rel(path):
        return str(path.relative_to(args.root))

    groups = [
        ("a", "game:: inside core/include/yrpp", a_hits),
        ("a2", "game namespace declarations or api/ includes inside core/include/yrpp", a2_hits),
        ("b", "core/src/yrpp includes drawing ABI (api/) or bridge headers", b_hits),
        ("b2", "info: core/src/yrpp includes internal drawing headers (not counted in b)", b2_hits),
        ("c", "bridge/src includes yrpp/ headers", c_hits),
        ("d", "presentation casts away const of a game object", d_hits),
        ("e", "presentation includes bridge, render, runtime or Godot headers", e_hits),
        ("f", "core/src/yrpp includes presentation headers", f_hits),
    ]
    status = 0
    if args.enforce:
        registered = json.loads((pathlib.Path(__file__).with_name("render_layers_exceptions.json")).read_text())
        for key, title, hits in groups:
            if key == "b2":
                continue
            allowed = registered.get(key, 0)
            if len(hits) > allowed:
                print(f"GUARD FAILED rule {key}: {len(hits)} hits, {allowed} registered ({title})")
                status = 1
            elif len(hits) < allowed:
                print(f"note: rule {key} has {len(hits)} hits, fewer than the {allowed} registered; lower the registration")
    if args.json:
        print(json.dumps({key: {"count": len(hits), "locations": [f"{rel(p)}:{n}: {t}" for p, n, t in hits]}
                          for key, _, hits in groups}, indent=2))
        return status
    for key, title, hits in groups:
        print(f"[{key}] {title}: {len(hits)}")
        per_file = {}
        for path, number, text in hits:
            per_file.setdefault(rel(path), []).append((number, text))
        for name, items in sorted(per_file.items()):
            print(f"    {name} ({len(items)})")
            for number, text in items:
                print(f"        {number}: {text}")
    print("SUMMARY " + " ".join(f"{g[0]}={len(g[2])}" for g in groups))
    return status


if __name__ == "__main__":
    sys.exit(main())
