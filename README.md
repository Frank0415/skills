# skills

Personal skill source repository. This repo stores skill source folders; link or copy individual skills into the runtime directories that each agent scans.

Each skill lives at the repository root:

```text
<skill-name>/
  SKILL.md              # required
  agents/openai.yaml    # optional, Codex UI/dependencies
  scripts/              # optional
  references/           # optional
  examples/             # optional
  assets/               # optional
```

Only `SKILL.md` is required. Add optional folders or files only when the skill actually needs them, and reference supporting files from `SKILL.md`.

Cross-compatible `SKILL.md` frontmatter:

```yaml
---
name: my-skill
description: Use when Codex should ...
---
```

Runtime locations:

- Codex personal skills: `/Users/franksair/.agents/skills/<skill-name>/SKILL.md`
- Codex repo skills: `<repo>/.agents/skills/<skill-name>/SKILL.md`
- Claude personal skills: `/Users/franksair/.claude/skills/<skill-name>/SKILL.md`
- Claude repo skills: `<repo>/.claude/skills/<skill-name>/SKILL.md`

Current local skill sources to import later:

- `/Users/franksair/.agents/skills`
- `/Users/franksair/.codex/skills` (skip `.system` unless intentionally vendoring system skills)
- `/Users/franksair/.claude/skills` if it exists
