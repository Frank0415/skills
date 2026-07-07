# AGENTS.md

- Keep each skill as one top-level folder named with lowercase letters, digits, and hyphens.
- Require `SKILL.md` in every skill folder. For OpenAI/Anthropic compatibility, keep YAML frontmatter to `name` and `description` unless a skill intentionally targets only one runtime.
- Add `agents/openai.yaml`, `scripts/`, `references/`, `examples/`, and `assets/` only when that skill needs them.
- Reference supporting files from `SKILL.md`; do not assume agents will discover their purpose.
- Do not add README, changelog, install notes, or extra docs inside a skill unless `SKILL.md` explicitly routes agents to read them.
- Keep edits scoped to the skill being changed.
- Validate changed skills with `quick_validate.py` when available.
