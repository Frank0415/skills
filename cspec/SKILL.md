---
name: cspec
description: "Write, rewrite, or review software specifications in controlled English or Simplified Chinese. Use for requirements, API contracts, design specs, runbooks, procedures, acceptance criteria, and configuration references. Triggers: cspec, controlled spec, MUST SHOULD MAY, 受控规范, 必须应该可以, 写规格, 验收条件. Detect the document language and apply the matching technique file."
---

# cspec

Write software specifications that have one stable interpretation. The contract is language-independent. Sentence craft is not: load the technique file for the document language.

- **English** → `references/english-techniques.md`
- **Simplified Chinese（简体中文）** → `references/chinese-techniques.md`
- **Mixed text** → the dominant language's file. Keep one keyword set for the whole document. Handle identifiers and quoted strings with that file's rules.

English sentence hygiene follows [SimpleEnglish](https://github.com/AminBlg/SimpleEnglish) (MIT). Chinese techniques implement the same intents with Chinese symptoms. Requirement force follows BCP 14; wording of the keywords lives in the language files.

## Procedure

1. Inspect code, schemas, interfaces, existing terms, and source documents. Reuse the project's established names.
2. Detect the document language and read the matching technique file before drafting.
3. Classify each passage as **normative**, **procedural**, or **descriptive**. Rewrite with the rules for that class.
4. Give every requirement a named actor, a condition, a direct verb, an observable result, and one requirement level.
5. Run the language file's self-check, then the completion check below.

## Outcome

A developer, reviewer, tester, and operator can identify:

- the actor or component;
- the required behavior;
- the condition that activates it;
- the observable result;
- the requirement level;`
- the evidence that verifies it.

Preserve technical detail. Simplify the language, not the system.

## Before writing

Resolve these when they affect the text:

- audience and document type;
- system boundary and named actors;
- authoritative terms and identifiers;
- units, formats, limits, defaults, and versions;
- normative behavior versus explanation;
- compatibility, safety, security, and data-loss constraints;
- acceptance or validation method.

Ask only for facts that cannot be discovered and would change the contract. Mark unknown values as `TBD` with an owner or decision question.

## Requirement levels

Every normative sentence has exactly one level. The language file supplies the words.

| Level | Force |
|---|---|
| Absolute requirement | Every conforming implementation performs this behavior. |
| Absolute prohibition | Every conforming implementation avoids this behavior. |
| Strong recommendation | Valid exceptions can exist; write the exception or decision criterion when it matters. |
| Permission | The behavior is allowed, not required. |
| Optional item | A feature, field, or step is available and not required. |

Use one keyword set through the whole document. Put a short convention near the beginning. Lowercase or unmarked words keep ordinary meaning.

Write absolute requirements for behavior every conforming implementation needs. Write recommendations only when a valid exception can exist. Write prohibitions for invalid states, real bans, and safety constraints. Write expected behavior in the positive when that is the contract.

## Data semantics

Name these states separately when they can occur:

- field absent;
- `null`;
- empty string `""`;
- empty array or empty object;
- value `0` or `false`.

For each applicable input or output, specify type, encoding, format, units, range, precision, default, requiredness, nullability, ordering, and duplicate rules.

## Behavioral contracts

For each applicable interface or behavior, specify:

- input type, format, encoding, units, range, nullability, and default;
- preconditions and authorization;
- trigger and input;
- state transition and ordering;
- output, return value, or externally visible side effect;
- error condition and exact error behavior;
- timeout, retry, cancellation, and idempotency;
- concurrency, consistency, and transaction boundary;
- compatibility or migration;
- validation method.

Include applicable dimensions only.

Error requirements name the condition and the observable result together: status code, error code, exception type, exit code, log level, or retry behavior, as the interface uses them.

## Code and identifiers

- Mark code in Markdown backticks or HTML `code`.
- Keep exact case and spelling for types, methods, fields, enum values, environment variables, filenames, paths, commands, and literals.
- Attach a normal noun to an identifier: `User` objects, the `close` method, the `READY` value.
- Pair HTTP methods with a noun: send a `POST` request; return an HTTP `401 Unauthorized` status code.
- Distinguish a filename from a file type, a command from its output, and UI text from identifiers.

Language-specific identifier phrasing is in the technique files.

## Procedures

- Number steps when order matters.
- Start each step with an imperative verb.
- Name the location when it is not obvious.
- Put independent actions in separate steps.
- Write commands and code in executable form.
- State the observable result when the next step depends on it.
- Put information in a `NOTE` or `备注`.
- Put a warning before the action that creates the hazard. Name the hazard, the consequence, and the prevention action. Lead with the command or condition, then the risk.

## Acceptance criteria

Make every criterion observable and repeatable. State inputs, initial state, action, and result. Use Given/When/Then when it improves the contract.

Examples follow the normative rule. Treat examples as non-normative unless the document says otherwise.

## Document structure

Keep the user's requested artifact and the repository template. For a new substantial specification, use the sections that help implement or verify the system:

```markdown
# Title

## Purpose and scope
## Terminology and normative language
## Actors and system boundary
## Requirements
## Interfaces and data
## State and failure behavior
## Acceptance criteria
## Open decisions
```

Assign a stable requirement ID to one requirement when the project needs traceability. Put rationale next to a requirement so it explains the decision and leaves the requirement's force unchanged.

## Rewrite and review

1. Keep source claims, constraints, technical detail, and requested structure.
2. Build a small terminology map.
3. Separate requirements, descriptions, procedures, examples, and rationale.
4. Complete actor, condition, verb, result, and level for each requirement.
5. Apply the language file, then normalize keywords and code formatting.
6. Check each requirement against implementation evidence, or label it proposed / `TBD`.
7. Confirm acceptance criteria cover the normative behavior.

**Review:** list ambiguities, contradictions, unverifiable requirements, missing states, and terminology conflicts. Cite section or line. Edit only when the user asks for a rewrite.

**Rewrite:** deliver the specification in the document language.

## Completion check

1. Each normative sentence has one level and one testable behavior.
2. Actors, terms, identifiers, units, conditions, and ranges are explicit.
3. Procedures keep execution order and use direct commands.
4. Error, state, and side-effect behavior is present where the interface needs it.
5. Examples agree with the normative text.
6. Source requirements and technical detail remain.
7. The language file's self-check has been run.

## Sources

- English techniques adapt [AminBlg/SimpleEnglish](https://github.com/AminBlg/SimpleEnglish) (MIT) and ASD-STE100 Issue 9 structural rules.
- BCP 14, RFC 2119 and RFC 8174: <https://www.rfc-editor.org/info/bcp14>
- ASD-STE100 Issue 9: <https://www.asd-ste100.org>
- GB/T 1.1—2020: <https://openstd.samr.gov.cn/bzgk/std/newGbInfo?hcno=C4BFD981E993C417EF475F2A19B681F1>
- Google developer documentation style: <https://developers.google.com/style>
- Microsoft procedures: <https://learn.microsoft.com/en-us/style-guide/procedures-instructions/>
