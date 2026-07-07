# Full-mode polish rules

Apply after `pdf2md convert` succeeds. Rewrite `document.md` in place.

Goal: study notes a human would keep — not a slide dump or OCR transcript.

## Lists

**Bullet character `•` → markdown `-`**

```markdown
# Before
• Spans several computers
• Has to deal with potential network issues

# After
- Spans several computers
- Has to deal with potential network issues
```

**Bare lines that are clearly list items → add `-`**

```markdown
# Before
Partition
Hard drive
LVM

# After
- Partition
- Hard drive
- LVM
```

**Numbered prose → ordered list**

```markdown
# Before
1 YARN scheduler allocates a container
2 Application master launched by the resource manager
3 Setup the tasks

# After
1. YARN scheduler allocates a container
2. Application master launched by the resource manager
3. Setup the tasks
```

**Lettered sub-steps → nested lists**

```markdown
# Before
6 Resources for
a Small tasks: run on the same node
b Large tasks: contact the resource manager for more containers
i Request resources for the maps (high priority)
ii Request resources for the reducers when enough maps have completed

# After
6. Resources for
   - Small tasks: run on the same node
   - Large tasks: contact the resource manager for more containers
     - Request resources for the maps (high priority)
     - Request resources for the reducers when enough maps have completed
```

Use `a.` / `b.` style only when order matters within a sub-level; otherwise prefer `-`.

## Headings

Keep `##` / `###` hierarchy. Merge duplicate headings on adjacent slides when they repeat the same section title.

## Continuity

- Join lines broken mid-sentence across slide boundaries
- Stitch logically attached things (like code blocks, program snippets, and logical prose) split across page/slide boundaries back together.
- Remove duplicate bullets that repeat the same idea
- Drop slide chrome (course name, date, "Thank you!") unless the user wants slide metadata
- Fix obvious OCR typos only when confident; do not guess technical terms

## Images

Keep every `![](images/...)` line. Do not add alt text or VLM captions.

## Quality bar

Before saving, ask:

1. Would this read well in Obsidian / Notion without editing?
2. Are lists valid markdown (not `•` or bare lines)?
3. Is the flow continuous — no random one-line fragments between sections?
4. Is technical content preserved verbatim where it matters?

## Lite mode

When user invoked `/pdf2md lite`, skip this entire file. Ship `document.md` unchanged after CLI.
