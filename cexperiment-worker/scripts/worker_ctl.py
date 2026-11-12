#!/usr/bin/env python3
"""
Execution & timer helper for cexperiment-worker:
  - calc-wakeup: exact 5*X single-job / 30-min multi-job wakeup interval calculator
  - pending:     list uncompleted spec pointers and readiness status
  - append-result: flock-safe atomic result-pointer registration (3-col & 5-col compatible)
"""

from __future__ import annotations

import argparse
import fcntl
import json
import os
import re
import sys
from pathlib import Path
from typing import Dict, List, Tuple


ROW_RE = re.compile(r"^\s*\|(.+)\|\s*$")
SEP_CELL_RE = re.compile(r"^:?-{2,}:?$")
MD_LINK_RE = re.compile(r"\[([^\]]+)\]\(([^)]+)\)")


def parse_queue(lines: List[str]) -> Tuple[int, List[Dict[str, str]]]:
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

        link_m = MD_LINK_RE.search(pointer)
        target = link_m.group(2).strip() if link_m else pointer

        entries.append(
            {
                "id": item_id,
                "type": row_type,
                "depends": depends,
                "resources": resources,
                "pointer": pointer,
                "target": target,
            }
        )

    return (num_cols or 5), entries


def compute_wakeup(remaining_mins: List[float]) -> Dict[str, object]:
    n = len(remaining_mins)
    if n == 0:
        return {
            "active_jobs": 0,
            "action": "STOP_ALL_TIMERS",
            "period_minutes": 0,
            "period_seconds": 0,
            "reason": "0 active jobs -> cancel/delete all scheduled timers immediately",
        }
    if n >= 2:
        return {
            "active_jobs": n,
            "action": "ARM_TIMER",
            "period_minutes": 30,
            "period_seconds": 1800,
            "cron_expression": "*/30 * * * *",
            "reason": f"{n} concurrent experiments active (>=2) -> fixed 30-minute wakeup",
        }

    t_rem = max(0.0, float(remaining_mins[0]))
    x = t_rem / 5.0
    if x < 15.0:
        period = 15
        clamp_note = f"X={x:.2f}m < 15m -> clamped up to 15m"
    elif x > 30.0:
        period = 30
        clamp_note = f"X={x:.2f}m > 30m -> clamped down to 30m"
    else:
        period = max(15, min(30, int(round(x))))
        clamp_note = f"15m <= X={x:.2f}m <= 30m -> {period}m"

    return {
        "active_jobs": 1,
        "action": "ARM_TIMER",
        "estimated_remaining_minutes": t_rem,
        "one_fifth_x_minutes": round(x, 2),
        "period_minutes": period,
        "period_seconds": period * 60,
        "cron_expression": f"*/{period} * * * *",
        "reason": f"1 active job (5*X={t_rem:.1f}m) -> {clamp_note}",
    }


def cmd_calc_wakeup(args: argparse.Namespace) -> int:
    res = compute_wakeup(args.remaining_mins)
    if args.json:
        print(json.dumps(res, ensure_ascii=False, indent=2))
    else:
        print(f"Action: {res['action']}")
        print(f"Period: {res['period_minutes']} min ({res['period_seconds']} s)")
        print(f"Reason: {res['reason']}")
    return 0


def cmd_pending(args: argparse.Namespace) -> int:
    qpath = Path(args.queue_file)
    if not qpath.exists():
        print(f"ERROR: Queue file not found: {qpath}", file=sys.stderr)
        return 1

    num_cols, entries = parse_queue(qpath.read_text(encoding="utf-8").splitlines())
    specs: Dict[str, Dict[str, str]] = {}
    completed = set()
    order: List[str] = []

    for e in entries:
        iid = e["id"]
        if iid not in order:
            order.append(iid)
        if e["type"] == "spec":
            specs[iid] = e
        elif e["type"] == "result":
            completed.add(iid)

    pending_items = []
    for iid in order:
        if iid in specs and iid not in completed:
            s = specs[iid]
            raw_deps = s["depends"].strip()
            if not raw_deps or raw_deps.lower() in {"none", "-", "n/a"}:
                deps: List[str] = []
            else:
                deps = [d.strip() for d in re.split(r"[,;\s]+", raw_deps) if d.strip()]
            unmet = [d for d in deps if d not in completed]
            spec_abs = str((qpath.parent / s["target"]).resolve())
            pending_items.append(
                {
                    "id": iid,
                    "spec_path": spec_abs,
                    "resources": s["resources"],
                    "table_depends": deps,
                    "table_depends_satisfied": len(unmet) == 0,
                    "unmet_table_depends": unmet,
                }
            )

    payload = {
        "queue_file": str(qpath.resolve()),
        "columns": num_cols,
        "completed_ids": [i for i in order if i in completed],
        "pending_count": len(pending_items),
        "pending": pending_items,
    }
    if args.json:
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        print(f"Completed ({len(payload['completed_ids'])}): {', '.join(payload['completed_ids']) or '(none)'}")
        print(f"Pending   ({len(pending_items)}):")
        for item in pending_items:
            dep_str = "ready (check spec header too)" if item["table_depends_satisfied"] else f"waiting on {item['unmet_table_depends']}"
            print(f"  - {item['id']}: {item['spec_path']} [{dep_str}]")
    return 0


def cmd_append_result(args: argparse.Namespace) -> int:
    qpath = Path(args.queue_file)
    if not qpath.exists():
        print(f"ERROR: Queue file not found: {qpath}", file=sys.stderr)
        return 1

    result_file = (qpath.parent / args.path).resolve()
    if not result_file.exists():
        print(
            f"ERROR: Result file {result_file} does not exist yet. "
            "Write and verify results/<ID>.md BEFORE appending its result pointer.",
            file=sys.stderr,
        )
        return 1

    with open(qpath, "r+", encoding="utf-8") as fh:
        fcntl.flock(fh.fileno(), fcntl.LOCK_EX)
        try:
            content = fh.read()
            num_cols, entries = parse_queue(content.splitlines())
            has_spec = any(e["id"] == args.id and e["type"] == "spec" for e in entries)
            has_result = any(e["id"] == args.id and e["type"] == "result" for e in entries)
            if not has_spec:
                print(f"ERROR: No spec pointer found for {args.id} in {qpath}", file=sys.stderr)
                return 1
            if has_result:
                print(f"ERROR: Result pointer for {args.id} already exists in {qpath}", file=sys.stderr)
                return 1

            link = f"[{args.label}]({args.path})"
            if num_cols >= 5:
                row = f"| {args.id} | result | - | - | {link} |\n"
            else:
                row = f"| {args.id} | result | {link} |\n"

            fh.seek(0, os.SEEK_END)
            if content and not content.endswith("\n"):
                fh.write("\n")
            fh.write(row)
            fh.flush()
            os.fsync(fh.fileno())
        finally:
            fcntl.flock(fh.fileno(), fcntl.LOCK_UN)

    print(f"Appended result pointer {args.id} -> {args.path}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Worker helper for cexperiment-worker")
    sub = parser.add_subparsers(dest="cmd", required=True)

    p_calc = sub.add_parser("calc-wakeup", help="Compute exact scheduled wakeup period")
    p_calc.add_argument(
        "--remaining-mins",
        type=float,
        nargs="*",
        default=[],
        help="Estimated remaining minutes for each currently active server experiment",
    )
    p_calc.add_argument("--json", action="store_true")
    p_calc.set_defaults(func=cmd_calc_wakeup)

    p_pend = sub.add_parser("pending", help="List uncompleted spec pointers in queue")
    p_pend.add_argument("queue_file")
    p_pend.add_argument("--json", action="store_true")
    p_pend.set_defaults(func=cmd_pending)

    p_res = sub.add_parser("append-result", help="Atomically append a result pointer row")
    p_res.add_argument("queue_file")
    p_res.add_argument("--id", required=True, help="Problem ID, e.g. C001")
    p_res.add_argument("--label", required=True, help="Link text, e.g. 'C001 Result'")
    p_res.add_argument("--path", required=True, help="Queue-relative path, e.g. results/C001.md")
    p_res.set_defaults(func=cmd_append_result)

    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
