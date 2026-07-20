---
name: cspec
description: Write controlled software specifications.
---

# cspec

Write, rewrite, or review software specifications in controlled technical English. Use this skill for requirements, API contracts, design specifications, runbooks, procedures, acceptance criteria, configuration references, and other documents where ambiguity can cause an implementation or operational error.

The style adapts Simplified Technical English (STE) principles to software work and adds normative language from BCP 14. It does not replace the ASD-STE100 standard or its controlled dictionary.

## Outcome

Produce a specification that has one stable interpretation. A developer, reviewer, tester, and operator must be able to identify:

- the actor or component;
- the required behavior;
- the condition that activates the behavior;
- the observable result;
- the requirement level;
- the evidence that verifies the requirement.

Preserve technical detail. Simplify the language, not the system.

## Before writing

Inspect the available code, schemas, interfaces, existing terminology, and source documents. Do not replace an established project term with a preferred synonym.

Resolve these items when they affect the text:

- audience and document type;
- system boundary and named actors;
- authoritative terms and identifiers;
- units, formats, limits, defaults, and versions;
- required behavior versus explanatory information;
- compatibility, safety, security, and data-loss constraints;
- acceptance or validation method.

Ask only for information that cannot be discovered and would change the contract. Mark unresolved facts as `TBD` with an owner or decision question. Do not invent a value to make the specification look complete.

## Normative keywords

For a document that uses BCP 14 terminology, include a short convention near the beginning and use these uppercase words only with their defined force:

- `MUST`: an absolute requirement.
- `MUST NOT`: an absolute prohibition.
- `SHOULD`: a strong recommendation for which valid exceptions can exist.
- `SHOULD NOT`: a discouraged behavior for which valid exceptions can exist.
- `MAY`: permission, not a requirement.
- `OPTIONAL`: a feature or field that is not required.

Do not use `MUST` for a preference. Do not use `SHOULD` when every conforming implementation needs the behavior. When a `SHOULD` has a material exception, state the exception or the decision criterion.

Avoid mixing `shall`, `must`, `is required to`, and `needs to` as equivalent requirement markers. Use the selected keyword set consistently. Lowercase words retain their ordinary English meaning unless the document explicitly defines another convention.

## Controlled English

### Vocabulary

- Use one term for one concept and one meaning for one term.
- Reuse the exact project term. Do not rotate synonyms for variety.
- Define a new term at first use or add it to a terminology table.
- Keep product names, API names, identifiers, commands, paths, literals, and units exact.
- Prefer a direct verb to a nominalization: use `validate the token`, not `perform token validation`.
- Prefer a common concrete word to a formal alternative when both meanings are equal.
- Do not use an acronym before defining it, unless the audience and project already treat it as the primary name.

### Sentences

- Give each descriptive sentence one topic.
- Give each normative sentence one independently testable requirement.
- Give each procedural sentence one instruction, except for actions that occur together.
- Use active voice when the actor is known. Name the actor when another actor could perform the action.
- Use imperative verbs for operator or developer procedures.
- Put a necessary condition before the action: `If the token is expired, the server MUST return...`.
- Put the action before its result in procedures.
- Split long logic into sentences or a vertical list. Do not omit a subject, verb, condition, or noun only to shorten the text.
- As an STE-oriented target, keep procedural sentences at 20 words or fewer and descriptive sentences at 25 words or fewer. Exclude code tokens from mechanical counting when splitting them would reduce accuracy.

### Ambiguity controls

- Replace unclear pronouns such as `it`, `this`, `that`, and `they` with the applicable noun.
- Replace vague references such as `the above`, `as needed`, `normally`, `appropriate`, `soon`, and `etc.` with a named object or measurable condition.
- State whether `or` is inclusive or exclusive when both readings are possible.
- State whether ranges include their endpoints.
- Attach each modifier and exception to the requirement it changes.
- Use positive statements for required behavior. Use negative statements for real prohibitions, invalid states, and safety constraints.
- Do not hide requirements in notes, examples, rationale, or introductions.

## Software specification rules

### Code and identifiers

- Put code elements in backticks in Markdown or `code` elements in HTML.
- Preserve exact case and spelling for classes, methods, fields, enum values, environment variables, filenames, paths, commands, and literals.
- Do not pluralize or inflect an identifier. Add a normal noun: `User` objects, the `close` method, the `READY` value.
- Use the established noun with an HTTP method: send a `POST` request; return an HTTP `401 Unauthorized` status code.
- Distinguish a filename from a file type and a command from its output.

### Behavioral contracts

For each applicable interface or behavior, specify:

- input type, format, encoding, units, range, nullability, and default;
- preconditions and authorization requirements;
- state transition and ordering constraints;
- output, return value, or externally visible side effect;
- error condition and exact error behavior;
- timeout, retry, cancellation, and idempotency behavior;
- concurrency and consistency behavior;
- compatibility or migration behavior;
- validation method.

Include only applicable dimensions. Do not add empty sections to imitate completeness.

### Procedures

- Use numbered steps when order matters.
- Start each step with an imperative verb.
- State where the action occurs when the location is not obvious.
- Separate independent actions into separate steps.
- Put commands and code in executable form.
- State the expected observable result after the action when the result is needed to continue safely.
- Keep information in a `NOTE`; do not put an instruction in a note.
- State a warning before the action that creates the hazard. Name the hazard, consequence, and prevention action.

### Acceptance criteria

Make every acceptance criterion observable and repeatable. State concrete inputs, initial state, action, and result. Use Given/When/Then only when it improves the contract; do not force all requirements into that format.

Examples are non-normative unless the document explicitly says otherwise. An example must agree with the normative rule and must not introduce a second behavior.

## Document structure

Preserve the user's requested artifact and established repository template. For a new substantial specification, use only the sections that help implement or verify the system:

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

Use requirement IDs when the project needs traceability. Keep each ID stable and assign it to one requirement. Do not use an ID for a heading that contains several independent requirements.

Separate normative requirements from rationale. A rationale can explain why a decision exists, but it cannot weaken or silently expand the requirement.

## Rewrite and review workflow

1. Preserve the source claims, constraints, and requested structure.
2. Build or infer a small terminology map.
3. Separate requirements, descriptions, procedures, examples, and rationale.
4. Rewrite each item with a named actor, direct verb, condition, and observable result.
5. Normalize normative keywords and code formatting.
6. Check every requirement against the implementation evidence or label it as proposed.
7. Remove repetition, synonym drift, vague qualifiers, and decorative prose.
8. Verify that acceptance criteria cover the normative behavior.

In review mode, lead with ambiguities, contradictions, unverifiable requirements, missing states, and terminology conflicts. Cite the exact section or line. Do not rewrite the document unless the user asks for edits.

## Example

Weak:

> The system should quickly handle invalid logins and provide an appropriate response.

Controlled:

> If a client sends an invalid password to `POST /sessions`, the service MUST return an HTTP `401 Unauthorized` status code. The response body MUST contain `code: "INVALID_CREDENTIALS"`. The service MUST NOT create a session.

## Completion check

Before delivery, confirm:

1. Each normative statement has one requirement level and one testable behavior.
2. Actors, terms, identifiers, units, conditions, and ranges are unambiguous.
3. Procedures preserve execution order and use direct commands.
4. Error, state, and side-effect behavior is present where required.
5. Examples agree with the normative text.
6. No source requirement or technical detail was lost during simplification.
7. The document does not claim ASD-STE100 compliance unless the project used the official standard, controlled dictionary, and its required compliance process.

## Reference basis

- ASD-STE100 Issue 9: <https://www.asd-ste100.org/assets/files/ASD-STE100_ISSUE9.pdf>
- BCP 14, RFC 2119 and RFC 8174: <https://www.rfc-editor.org/info/bcp14>
- Google developer documentation style guide: <https://developers.google.com/style>
- Microsoft procedures and instructions: <https://learn.microsoft.com/en-us/style-guide/procedures-instructions/>
