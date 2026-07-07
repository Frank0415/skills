#!/usr/bin/env python3
"""Validate pdf2md convert output directory."""

from __future__ import annotations

import json
import sys
from pathlib import Path

REQUIRED_FILES = ("document.md", "security_report.json", "meta.json")
BLOCKING = frozenset({"HIGH", "CRITICAL"})


def validate(out_dir: Path) -> list[str]:
    errors: list[str] = []
    if not out_dir.is_dir():
        return [f"not a directory: {out_dir}"]

    for name in REQUIRED_FILES:
        if not (out_dir / name).is_file():
            errors.append(f"missing {name}")

    report_path = out_dir / "security_report.json"
    if report_path.is_file():
        try:
            report = json.loads(report_path.read_text(encoding="utf-8"))
        except json.JSONDecodeError as exc:
            errors.append(f"invalid security_report.json: {exc}")
        else:
            severity = str(report.get("overall_severity", "NONE")).upper()
            if severity in BLOCKING:
                errors.append(
                    f"overall_severity is {severity} — do not ingest without user-approved --force"
                )

    if not (out_dir / "images").is_dir():
        errors.append("missing images/ directory")

    return errors


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} <output-dir>", file=sys.stderr)
        return 1

    errors = validate(Path(sys.argv[1]))
    if errors:
        for err in errors:
            print(f"FAIL: {err}")
        return 1

    print("OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
