---
name: cslides-orchestrator
description: Orchestrate multiple course-slide PDFs through independent cslides workers. Use for PDF folders, batches, or archives; keep one PDF per worker and run separate convert, judge, and fix stages with bounded concurrency.
---

# cslides-orchestrator

Alias: `cslides_orchestrator`.

Use this skill when the user provides multiple course-slide PDFs, a directory of PDFs, or a zip containing many PDFs and wants them converted with `cslides`.

This skill does not write course HTML itself. It orchestrates independent Codex CLI workers. Each conversion worker receives exactly one PDF path and uses `cslides` for that PDF only.

## Execution profile

Use this default profile for every worker:

- Maximum active workers: `3`.
- Model: `gpt-5.6-sol`.
- Reasoning effort: `medium`.

Apply the same model and effort to convert, judge, fix, and rejudge workers. Do not mix worker profiles within one run. Only an explicit user instruction in the current request may override these defaults.

## Preflight interview

Before starting workers, resolve concurrency with the user in a `grill-me`-style interview:

- Ask one question at a time.
- Provide the recommended answer.
- If the user already specified an answer in the current request, do not ask it again.
- If the user says to use defaults, use three active workers without further questions.

Required question:

1. Ask how many active subagents the user wants.
   - Recommended and default answer: `3`.
   - This number is the total active Codex CLI worker count, including convert, judge, fix, and rejudge workers.

Do not ask about model or reasoning effort unless the user explicitly requests a different profile. Do not dispatch any Codex CLI worker until concurrency is resolved, unless the user says to use defaults.

When launching workers:

- Pass `-m gpt-5.6-sol`.
- Pass `-c model_reasoning_effort="medium"`.
- If the user explicitly overrides the profile, pass the requested model and effort to every worker instead.

## Hard rules

- Do not use computer-use, browser UI, Chrome UI, or any GUI automation. Codex CLI workers cannot rely on those tools.
- Do not create a Python, shell, or Node script that loops over PDFs and writes HTML files.
- Do not batch multiple PDFs into one cslides prompt.
- Do not ask one judge or fix worker to evaluate multiple PDFs.
- Keep the active worker count at the preflight-resolved limit.
- A PDF is complete only after convert succeeds and judge returns `PASS`.

## Prerequisite check

Before orchestration, check:

```bash
command -v codex
test -f "$HOME/.agents/skills/cslides/SKILL.md" || \
  test -f "${CODEX_HOME:-$HOME/.codex}/skills/cslides/SKILL.md"
```

If `codex` is missing, cannot start, or `cslides` is not installed, fall back to serial processing in the current agent:

1. Select one PDF from the queue.
2. Use `cslides` on that PDF only.
3. Judge that HTML before starting the next PDF.
4. Continue one PDF at a time.

## Queue setup

1. Resolve all candidate PDFs from the user input.
2. Sort paths naturally by filename unless the user gives an explicit order.
3. For each PDF, derive the target HTML as the same path with `.html`.
4. Never dispatch two workers for the same PDF at the same time.

## Convert worker prompt

Send this prompt to one Codex CLI instance per PDF:

```text
Use the cslides skill.

Convert exactly this PDF to a single-file HTML study guide:
<PDF_PATH>

Rules:
- Process only this PDF.
- Write the HTML next to the PDF with the same basename and .html extension.
- Do not process any other PDF.
- Do not run a loop over PDFs.
- Do not use computer-use, browser UI, Chrome UI, or GUI automation.
- Follow cslides fully: read every slide, inspect visual content, create and delete the temporary page audit, preserve every original slide image, and write real teaching explanations instead of template text.
- Do not stop at page/image coverage. Explain formulas, examples, algorithms, tables, diagrams, transitions, and why pages belong in each unit.

Final response must include:
- HTML path
- PDF page count
- HTML slide image count
- whether temporary audit files were removed
- any known quality caveat
```

For a plain conversion command, the useful payload is just `cslides + <PDF_PATH>` plus the rules above.

## Judge worker prompt

After a convert worker finishes, send this prompt to a separate Codex CLI instance:

```text
Use the cslides skill as the quality standard.

Judge this generated HTML against its source PDF. Do not edit files.

PDF:
<PDF_PATH>

HTML:
<HTML_PATH>

Check:
- every PDF page appears exactly once as an original slide image in the HTML
- unit grouping keeps complete concepts, formulas, examples, algorithm traces, and visual progressions together
- explanations are specific to the course content, not generic template text
- formulas, recurrences, complexity claims, tables, pseudocode, diagrams, and examples are actually explained
- transitions between units are meaningful and not boilerplate
- no stale audit files, local image paths, placeholder text, or naked display-math delimiters remain

Return exactly one verdict line first:
PASS
or
FAIL

Then list concrete issues with file path, unit id, pages, and a short reason.
```

## Fix worker prompt

If judge returns `FAIL`, send this prompt to a new Codex CLI instance:

```text
Use the cslides skill.

Fix the generated HTML for exactly this PDF. Do not process any other PDF.

PDF:
<PDF_PATH>

HTML:
<HTML_PATH>

Judge findings:
<JUDGE_FINDINGS>

Rules:
- Re-read the PDF pages needed for the failing units.
- Preserve correct embedded slide images and navigation when possible.
- Rewrite weak units into real teaching explanations.
- Repair unit grouping when an example, formula derivation, algorithm trace, or visual progression was split incorrectly.
- Explain formulas, recurrences, complexity, pseudocode, diagrams, and examples directly.
- Remove generic template language.
- Do not use computer-use, browser UI, Chrome UI, or GUI automation.
- Do not touch other PDFs or HTML files.

Final response must include:
- changed HTML path
- fixed units/pages
- verification summary
```

Run judge again after each fix. If the same PDF fails twice, stop retrying that PDF and report it as needing manual review.

## Monitoring loop

Every 3 minutes:

1. Poll active Codex CLI instances.
2. If a convert worker finished, start its judge worker when a slot is free.
3. If a judge returns `PASS`, mark the PDF complete.
4. If a judge returns `FAIL`, start one fix worker when a slot is free.
5. If a fix worker finished, start judge again.
6. If a slot is free and queued PDFs remain, start the next convert worker.

Keep at most the resolved active subagent count.

## Final report

Report:

- total PDFs discovered
- completed PDFs
- failed PDFs and why
- HTML paths
- whether fallback mode was used
