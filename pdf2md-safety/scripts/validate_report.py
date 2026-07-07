#!/usr/bin/env python3
"""Validate pdf2md scan output (security report only)."""

from __future__ import annotations

import json
import sys
from pathlib import Path

REQUIRED_KEYS = ("structural", "ocr_diff", "overall_severity")


def validate(out_dir: Path) -> list[str]:
    errors: list[str] = []
    if not out_dir.is_dir():
        return [f"not a directory: {out_dir}"]

    report_path = out_dir / "security_report.json"
    if not report_path.is_file():
        return ["missing security_report.json"]

    try:
        report = json.loads(report_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        return [f"invalid security_report.json: {exc}"]

    for key in REQUIRED_KEYS:
        if key not in report:
            errors.append(f"missing key: {key}")

    severity = str(report.get("overall_severity", "")).upper()
    if severity not in {"NONE", "LOW", "MEDIUM", "HIGH", "CRITICAL"}:
        errors.append(f"unexpected overall_severity: {severity!r}")

    return errors


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} <scan-out-dir>", file=sys.stderr)
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
