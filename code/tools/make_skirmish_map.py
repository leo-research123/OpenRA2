#!/usr/bin/env python3
"""Derive a skirmish map from a scenario map.

Keeps terrain, overlay, lighting and the first N waypoints as multiplayer
start positions; drops scenario Houses, objects and trigger logic, which the
skirmish setup creates from the lobby instead.

    make_skirmish_map.py SOURCE.map OUTPUT.map --name "Name" [--starts 30]
"""
import argparse
import math
import re
import sys

KEEP = {"map", "lighting", "terrain", "smudge", "overlaypack", "overlaydatapack", "isomappack5"}


def sections(text):
    current, body = None, []
    for line in text.splitlines():
        header = re.match(r"^\s*\[([^\]]+)\]\s*$", line)
        if header:
            if current is not None:
                yield current, body
            current, body = header.group(1), []
        elif current is not None:
            body.append(line)
    if current is not None:
        yield current, body


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("source")
    parser.add_argument("output")
    parser.add_argument("--name", required=True)
    parser.add_argument("--starts", type=int, default=30)
    parser.add_argument("--min-spacing", type=float, default=20.0, help="minimum cell distance between starts")
    args = parser.parse_args()

    with open(args.source, encoding="latin-1") as file:
        source = dict((name, body) for name, body in sections(file.read()))
    missing = [name for name in ("Map", "IsoMapPack5", "Waypoints") if name not in source]
    if missing:
        sys.exit(f"source map lacks sections: {', '.join(missing)}")

    waypoints = {}
    for line in source["Waypoints"]:
        key, _, value = line.partition("=")
        if key.strip().isdigit() and value.strip():
            waypoints[int(key)] = int(value)
    starts = []
    for index in range(args.starts):
        if index not in waypoints:
            sys.exit(f"source map has no waypoint {index}")
        cell = waypoints[index]
        starts.append((cell % 1000, cell // 1000))
    closest = min(math.dist(a, b) for i, a in enumerate(starts) for b in starts[i + 1:])
    if closest < args.min_spacing:
        sys.exit(f"starts are only {closest:.1f} cells apart")

    out = [
        "[Basic]",
        f"Name={args.name}",
        "NewINIFormat=4",
        "MultiplayerOnly=1",
        "Official=no",
        "",
    ]
    for name, body in source.items():
        if name.lower() not in KEEP:
            continue
        out.append(f"[{name}]")
        out.extend(line for line in body if line.strip())
        out.append("")
    out.append("[Waypoints]")
    out.extend(f"{index}={waypoints[index]}" for index in range(args.starts))
    out.append("")
    with open(args.output, "w", encoding="latin-1", newline="\r\n") as file:
        file.write("\n".join(out))
    print(f"{args.output}: {args.starts} starts, closest pair {closest:.1f} cells apart")


if __name__ == "__main__":
    main()
