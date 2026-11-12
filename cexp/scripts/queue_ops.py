#!/usr/bin/env python3
"""
flock-safe pointer queue utility for Commander (cexp).
Supports both 5-column parallel-aware pointer tables and 3-column legacy pointer tables.
"""

from __future__ import annotations

import argparse
import fcntl
import json
import os
import re
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple


ROW_RE = re.compile(r"^\s*\|(.+)\|\s*$")
SEP_CELL_RE = re.compile(r"^:?-{2,}:?$")
ID_NUM_RE = re.compile(r"^([A-Za-z]+)(\d+)$")


def parse_queue_lines(lines: List[str]) -> Tuple[int, List[Dict[str, str]]]:
    """Return (num_columns, parsed_entries)."""
    num_cols = 0
    header_seen = False
    entries: List[Dict[str, str]] = []

    for raw in lines:
        m = ROW_RE.match(raw)
        if not m:
            continue
        cells = [c.strip() for c in m.group(1).split("|")]
        if all(SEP_CELL_RE.match(c) for c in cells if c):
            continue
        if not header_seen:
            header_seen = True
            num_cols = len(cells)
            continue

        if len(cells) < 3:
            continue
        item_id = cells[0]
        row_type = cells[1].lower()
        if len(cells) >= 5:
            depends = cells[2]
            resources = cells[3]
            pointer = cells[4]
        else:
            depends = ""
            resources = ""
            pointer = cells[2]

        entries.append(
            {
                "id": item_id,
                "type": row_type,
                "depends": depends,
                "resources": resources,
                "pointer": pointer,
            }
        )

    return (num_cols or 5), entries


def compute_next_id(existing_ids: List[str], default_prefix: str = "E") -> str:
    prefix = default_prefix
    max_num = 0
    width = 3
    for item_id in existing_ids:
        m = ID_NUM_RE.match(item_id)
        if m:
            prefix = m.group(1)
            width = max(width, len(m.group(2)))
            max_num = max(max_num, int(m.group(2)))
    return f"{prefix}{max_num + 1:0{width}d}"


def summarize(queue_path: Path) -> Dict[str, object]:
    text = queue_path.read_text(encoding="utf-8")
    num_cols, entries = parse_queue_lines(text.splitlines())

    specs: Dict[str, Dict[str, str]] = {}
    results: Dict[str, Dict[str, str]] = {}
    ordered_ids: List[str] = []

    for e in entries:
        iid = e["id"]
        if iid not in ordered_ids:
            ordered_ids.append(iid)
        if e["type"] == "spec":
            specs[iid] = e
        elif e["type"] == "result":
            results[iid] = e

    completed = [i for i in ordered_ids if i in specs and i in results]
    pending = [i for i in ordered_ids if i in specs and i not in results]

    return {
        "queue_file": str(queue_path.resolve()),
        "columns": num_cols,
        "total_specs": len(specs),
        "completed_ids": completed,
        "pending_ids": pending,
        "next_suggested_id": compute_next_id(ordered_ids),
        "specs": specs,
        "results": results,
    }


def cmd_init(args: argparse.Namespace) -> int:
    qpath = Path(args.queue_file)
    if qpath.exists() and not args.force:
        print(
            f"ERROR: Queue file already exists at {qpath}. "
            "Reuse existing same-series queue by default, or pass --force only when explicitly creating a new queue.",
            file=sys.stderr,
        )
        return 1

    qpath.parent.mkdir(parents=True, exist_ok=True)
    if args.columns == 3:
        body = (
            f"# {args.title}\n\n"
            f"Shared Contract: [{Path(args.contract).name}]({args.contract})\n\n"
            "| ID | Type | Pointer |\n"
            "| --- | --- | --- |\n"
        )
    else:
        body = (
            f"# {args.title}\n\n"
            f"Shared Contract: [{Path(args.contract).name}]({args.contract})\n\n"
            "| ID | Type | Depends | Resources | Pointer |\n"
            "| --- | --- | --- | --- | --- |\n"
        )
    qpath.write_text(body, encoding="utf-8")
    print(f"Initialized {args.columns}-column queue at {qpath.resolve()}")
    return 0


def cmd_status(args: argparse.Namespace) -> int:
    qpath = Path(args.queue_file)
    if not qpath.exists():
        print(f"ERROR: Queue file not found: {qpath}", file=sys.stderr)
        return 1
    info = summarize(qpath)
    if args.json:
        print(json.dumps(info, ensure_ascii=False, indent=2))
    else:
        print(f"Queue:     {info['queue_file']} ({info['columns']}-col)")
        print(f"Specs:     {info['total_specs']}")
        print(f"Completed: {', '.join(info['completed_ids']) or '(none)'}")
        print(f"Pending:   {', '.join(info['pending_ids']) or '(none)'}")
        print(f"Next ID:   {info['next_suggested_id']}")
    return 0


def cmd_append_spec(args: argparse.Namespace) -> int:
    qpath = Path(args.queue_file)
    if not qpath.exists():
        print(f"ERROR: Queue file not found: {qpath}", file=sys.stderr)
        return 1

    with open(qpath, "r+", encoding="utf-8") as fh:
        fcntl.flock(fh.fileno(), fcntl.LOCK_EX)
        try:
            content = fh.read()
            num_cols, entries = parse_queue_lines(content.splitlines())
            for e in entries:
                if e["id"] == args.id and e["type"] == "spec":
                    print(
                        f"ERROR: Spec row for {args.id} already exists. Append a new ID instead of overwriting.",
                        file=sys.stderr,
                    )
                    return 1

            link = f"[{args.label}]({args.path})"
            if num_cols >= 5:
                row = f"| {args.id} | spec | {args.depends} | {args.resources} | {link} |\n"
            else:
                row = f"| {args.id} | spec | {link} |\n"

            fh.seek(0, os.SEEK_END)
            if content and not content.endswith("\n"):
                fh.write("\n")
            fh.write(row)
            fh.flush()
            os.fsync(fh.fileno())
        finally:
            fcntl.flock(fh.fileno(), fcntl.LOCK_UN)

    print(f"Appended spec pointer {args.id} -> {args.path}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="flock-safe pointer queue helper for cexp")
    sub = parser.add_subparsers(dest="cmd", required=True)

    p_init = sub.add_parser("init", help="Initialize a new pointer queue file")
    p_init.add_argument("queue_file")
    p_init.add_argument("--title", default="Experiment Work Queue (Pointer Table)")
    p_init.add_argument("--contract", default="specs/00-contract.md")
    p_init.add_argument("--columns", type=int, choices=[3, 5], default=5)
    p_init.add_argument("--force", action="store_true")
    p_init.set_defaults(func=cmd_init)

    p_status = sub.add_parser("status", help="Inspect completed and pending pointers")
    p_status.add_argument("queue_file")
    p_status.add_argument("--json", action="store_true")
    p_status.set_defaults(func=cmd_status)

    p_app = sub.add_parser("append-spec", help="Atomically append a spec pointer row")
    p_app.add_argument("queue_file")
    p_app.add_argument("--id", required=True, help="Problem ID, e.g. C005 or E001")
    p_app.add_argument("--label", required=True, help="Link text for the spec pointer")
    p_app.add_argument("--path", required=True, help="Relative path to spec markdown")
    p_app.add_argument("--depends", default="none", help="Prerequisite IDs or 'none' (5-col tables)")
    p_app.add_argument("--resources", default="1xGPU", help="Short resource summary (5-col tables)")
    p_app.set_defaults(func=cmd_append_spec)

    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
