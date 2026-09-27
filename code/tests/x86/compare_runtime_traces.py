"""Compare real Windows bootstrap boundary traces from the two test-only DLLs.

The oracle calls the original 0x5301A0; the candidate calls the replacement.
Both use actual original game objects and file services. This comparison does
not by itself certify gameplay, teardown or every failure branch.
"""
import argparse
import hashlib
import json
from pathlib import Path


def load(path):
    records = [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines() if line.strip()]
    if [r["phase"] for r in records] != ["before", "after"]:
        raise ValueError(f"{path}: expected exactly one before/after pair")
    if records[1]["success"] is not True:
        raise ValueError(f"{path}: bootstrap returned failure")
    return records


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original", type=Path, required=True)
    parser.add_argument("--replacement", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    original, replacement = load(args.original), load(args.replacement)
    native_counts = None
    if any("native_raw_checks" in r for r in original + replacement):
        native_counts = [r["native_raw_checks"] for r in replacement]
        if [r["native_raw_checks"] for r in original] != [0, 0] or native_counts != [0, 100]:
            raise ValueError("expected original RawFile count 0 and replacement native RawFile count 100")
    for before, after in zip(original, replacement):
        if {k: v for k, v in before.items() if k != "native_raw_checks"} != {
                k: v for k, v in after.items() if k != "native_raw_checks"}:
            differences = [k for k in before["state"] if before["state"][k] != after["state"].get(k)]
            raise ValueError(f"{before['phase']}: runtime state mismatch: {differences}")
    report = {
        "kind": "real Windows original/replacement bootstrap boundary comparison",
        "shared_stub_callees": False,
        "instrumented_test_dlls": True,
        "before_equal": True,
        "after_equal": True,
        "success": True,
        "native_raw_checks": native_counts,
        "phases": [],
    }
    for record in replacement:
        state = record["state"]
        canonical = json.dumps(state, sort_keys=True, separators=(",", ":")).encode()
        report["phases"].append({
            "phase": record["phase"],
            "disk": state["disk"],
            "mixes": [m["name"] for m in state["mixes"]],
            "index_entries": sum(m["count"] for m in state["mixes"]),
            "generic_slots_compared": len(state["slots"]),
            "expansions": state["expansions"],
            "expansion_capacity": state["expansion_capacity"],
            "state_sha256": hashlib.sha256(canonical).hexdigest(),
        })
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print("PASS: real before/after MIX links, complete indexes, flags, slots, expansion array, disk and return value")


if __name__ == "__main__":
    main()
