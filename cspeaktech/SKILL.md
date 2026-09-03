---
name: cspeaktech
description: Explain an existing technical specification or proposed change in plain language without losing precision. Use for standalone explanations of the problem, solution, behavior, and schema changes; not for authoring requirements or general prose rewriting.
---

# cspeaktech

Turn the referenced specification or change into an accurate mental model for a
technical reader who does not live in the code. Simplify the telling, never the
claims.

Use the language requested by the user. Otherwise, use the primary language of
the referenced artifact.

## Scope

This skill explains an existing specification, diff, or proposed change. It does
not replace `cspec` for writing normative software contracts or `cspeak` for
general reader-facing prose.

Assume the reader is technical and can understand pseudocode and precise claims,
but has read none of the code, diff, specification, transcript, or project-local
labels.

## Register

- Walk one concrete scenario end to end: the triggering event, what happens
  today, and what the change or unbuilt alternative would do. Do not rely on an
  abstract list of properties.
- Define every term of art at first use. Do not rely on labels invented by the
  specification, code, or session.
- Use small concrete examples without replacing precise claims with analogies.
- Use decision-level pseudocode to explain control flow, ordering, or timing.
  Show conditions and their order, not real function signatures. If prose uses
  phrases such as “only when,” “before,” “unless,” or “as soon as,” the reader
  must still be able to predict what happens on a repeated or competing call.
- Make the explanation stand alone. It fails if the reader needs the original
  diff, specification, or transcript to understand it.

## Workflow

1. Read the complete referenced artifact. Inspect current owners when the
   artifact alone cannot establish behavior. If no artifact is named, use the
   current working-tree or branch change.
2. Separate behavior that exists today from behavior that is only proposed.
3. Explain the problem through its user or operational consequence, including
   why the current design produces it.
4. Explain the solution as one simple before-and-after data flow. Introduce each
   component by responsibility, not by filename or internal symbol.
5. Inventory schema and durable-contract changes exhaustively: added, changed,
   removed, reset, and deliberately unchanged. Include cursor and wire-format
   cutovers when they affect stored data or readers. State explicitly when there
   are no schema changes.

## Output

Use these semantic sections in order, localizing the headings when appropriate:

1. `Problem`
2. `Solution`
3. `Schema changes`

End with one short sentence stating what users should notice after the change.

## Rules

- Lead with behavior and boundaries. Mention implementation names only when they
  clarify ownership or a contract.
- Distinguish canonical records from derived indexes, caches, summaries, and
  presentation grouping.
- State destructive resets, migration requirements, compatibility behavior,
  eventual consistency, and intentional data loss directly.
- Do not omit a schema change because it is operational rather than user-visible.

## Source

Adapted from [`dzhng/skills`' `eli5`](https://github.com/dzhng/skills/tree/main/skills/engineering/eli5)
under the MIT License.
