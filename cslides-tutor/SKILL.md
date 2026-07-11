---
name: cslides-tutor
description: Answer student follow-ups and write them into existing cslides HTML. Use for slide questions, derivations, examples, or corrections; preserve source context, MathJax, styling, and closed blocks.
---

# CSlides Tutor

Extend an existing course HTML after its initial conversion. Treat the conversational answer and the HTML update as one operation: answer the student, then persist the same explanation beside the concept that caused the question.

## Hard boundaries

- Work on one explicitly resolved lecture HTML at a time. Do not turn this into a multi-PDF conversion workflow.
- Modify an additional HTML variant only when the user requested synchronization or the current conversation already established that the files are synchronized variants. Compare their relevant structures before editing both.
- Preserve the existing information architecture, CSS system, slide images, navigation, scripts, and user edits. Do not regenerate the whole lecture for a local follow-up.
- Do not stop after answering in chat. Unless the user explicitly asks for explanation only, write the question and answer into the target HTML in the same turn.
- Do not commit, push, stage, or install the skill unless the user explicitly requests that action.

## 1. Resolve the real target

1. Resolve the HTML path from the user's explicit path, `file://` URL, selected-file context, or the latest unambiguous target in the conversation.
2. Read the target before editing. Identify:
   - the relevant module, unit, slide page, and existing anchor;
   - the existing question-block markup and CSS classes;
   - the MathJax or KaTeX rendering contract;
   - whether all `<details>` blocks or only student-question blocks are expected to start closed.
3. Locate the source PDF when the question names a slide page or depends on visual evidence. Prefer a same-basename PDF near the HTML; otherwise use the source already established in the conversation.
4. Inspect the exact source page's extracted text and rendered image when a formula, diagram, table, or page wording matters. Do not answer page-specific questions from memory alone.

Ask for clarification only when the target remains genuinely ambiguous after local inspection.

## 2. Decide whether to add or revise

- Add a new question block when the student introduces a distinct conceptual gap.
- Revise the existing block when the new message corrects, narrows, or continues the same question. Do not leave contradictory answers in separate blocks.
- Split a new block when the follow-up has become a standalone prerequisite or a new procedure that should be found independently.
- Place the block beside the concept it explains, normally after the relevant commentary or source-page explanation and before that unit's mastery summary.
- Do not append every question to the top or bottom of the document.
- Keep one coherent derivation or worked example together. Do not scatter its steps across several blocks.

## 3. Write the answer as teaching material

Start with the direct answer, then supply only the dependencies needed to make it defensible.

For mathematical or algorithmic questions:

1. Restate the student's confusion precisely.
2. Define every newly used symbol and matrix dimension.
3. Show the dependency chain: which known fact implies which next step.
4. Derive the disputed formula rather than repeating it.
5. Include a complete numerical example when the issue is computational.
6. State the common mistake or competing convention when it explains the confusion.
7. Reconnect the result to the original slide and the surrounding lecture topic.

Distinguish these content types explicitly when relevant:

- `课件原意`: directly supported by the slide.
- `推导`: follows from the slide's formulas or standard algebra.
- `上下文补足`: standard prerequisite omitted by the slide.
- `修正`: corrects a prior explanation or a slide ambiguity.

Keep the prose direct and student-facing. Preserve English technical terms when useful, but explain them in Chinese. Avoid generic lecture-template language.

## 4. Match the existing HTML contract

Reuse the target's existing classes and component hierarchy. Prefer its current student-question pattern. When no pattern exists, use this minimal structure and adapt class names to the page:

```html
<details class="supplement" id="qa-p020-householder-example" data-cslides-tutor="qna">
  <summary>
    学生追问: P020 · Householder reflection 怎么算
    <span class="supplement-kind">学生追问 / QR</span>
  </summary>
  <div class="supplement-body">
    <p><strong>问题:</strong> Householder reflection 这个公式是什么意思？</p>
    <!-- Direct answer, derivation, dimensions, and worked example. -->
  </div>
</details>
```

Rules:

- Give every new block a stable, descriptive, unique ID.
- Add `data-cslides-tutor="qna"` when it does not conflict with the existing markup. The audit script also recognizes older summaries beginning with `学生追问:`.
- Leave every student-question `<details>` closed by default. Never add the `open` attribute.
- If the page already requires all blocks to start closed, preserve that site-wide rule.
- Do not put cards inside cards or add a new visual hierarchy for one answer.
- Keep spacing dense and functional. Reuse existing responsive table and formula containers.
- Update navigation or counters only when the current page already indexes student-question blocks.

## 5. Preserve mathematical rendering

Inspect the complete rendering path before writing formulas. Separate three layers when debugging:

1. **HTML source:** entities such as `&amp;` and `&lt;` must be valid inside attributes.
2. **DOM attribute value:** `element.getAttribute("data-tex")` decodes those entities back to TeX characters such as `&` and `<`.
3. **MathJax output:** MathJax converts the decoded TeX into `mjx-container` nodes. A valid-looking source string is not enough; verify the rendered result.

### Follow the page's established formula contract

- If the page uses `.math-text[data-tex]` and `.math-display[data-tex]`, use those wrappers consistently instead of mixing in raw delimiters.
- Use `.math-text` for symbols and short inline relations. Use `.math-display` for derivations, matrices, and equations that need their own line.
- Keep fallback text semantically identical to `data-tex`. The fallback remains visible before MathJax finishes and when the renderer fails.
- Do not wrap fallback text in `\(...\)` or `\[...\]` when a page script already reads `data-tex` and calls `tex2svgPromise` or `typesetPromise`.
- Keep Chinese prose outside TeX where possible. Use `\text{...}` only for short mathematical labels; long Chinese sentences inside MathJax are harder to size and wrap reliably.
- Define matrix dimensions in nearby prose or formula wrappers. Do not rely on visual shape inference when the question concerns mappings, bases, SVD sides, or coordinate changes.

Use source markup like this when it matches the target page:

```html
<span class="math-text" data-tex="Q^\top Q=I">Q^\top Q=I</span>

<div class="math-display formula-wide"
     data-tex="A=\begin{pmatrix}1&amp;2\\3&amp;4\end{pmatrix}">
  A=\begin{pmatrix}1&amp;2\\3&amp;4\end{pmatrix}
</div>
```

### Escape TeX correctly inside HTML attributes

- Write matrix column separators as `&amp;` in HTML source. The browser decodes them to `&` before MathJax sees the TeX.
- Write comparisons as `&lt;` and `&gt;` when they occur inside quoted attributes.
- Keep TeX row separators as `\\` in the HTML source. Do not apply JavaScript-string escaping rules to ordinary HTML markup.
- Avoid literal quote characters that terminate `data-tex`. Prefer `\text{label}` or move the label into surrounding HTML.
- Balance every `{` with `}` and every `\begin{environment}` with the matching `\end{environment}`.
- Preserve the exact TeX command spelling. Broken text such as `\mathbb K`, `\mathcal B\prime`, or a missing backslash often renders as ordinary letters instead of raising an obvious error.

### Diagnose `Misplaced &` instead of hiding it

`Misplaced &` normally means MathJax received an alignment separator where no alignment environment was active. Check these causes in order:

1. A matrix lost its surrounding `\begin{pmatrix}...\end{pmatrix}` or another aligned environment.
2. A matrix column separator was written as a raw `&` in the HTML attribute instead of source-level `&amp;`.
3. A quote or malformed tag truncated `data-tex`, so MathJax received only part of the matrix.
4. The fallback text and `data-tex` differ, making the visible source appear correct while the rendered attribute is broken.

Do not fix this by deleting ampersands. Restore the matrix environment, HTML escaping, and complete attribute value.

### Handle long formulas without breaking the page

- Put long equations in the page's existing wide class, such as `.formula-wide`.
- Ensure the formula container has local horizontal overflow, usually `overflow-x:auto` or `overflow:auto`.
- Keep tables independently scrollable on narrow screens. A wide matrix inside a table must not widen the entire document.
- Do not reduce all formula font sizes to make one equation fit. Preserve readability and allow local scrolling.

### Verify asynchronous MathJax rendering

After reload, wait for the renderer rather than checking immediately. For pages using MathJax SVG output, compare:

```js
const sourceCount = document.querySelectorAll(
  '.math-text[data-tex], .math-display[data-tex]'
).length;
const renderedCount = document.querySelectorAll('mjx-container').length;
const errorCount = document.querySelectorAll('mjx-merror, merror').length;
```

Require `renderedCount === sourceCount` and `errorCount === 0` after MathJax finishes. Also inspect console errors and the exact changed block. A global count can pass while one formula is clipped or placed in the wrong container.

## 6. Edit narrowly

- Use `apply_patch` for authored content edits. Use a structured or mechanical insertion only when the target is minified and the authored fragment was already reviewed.
- Preserve unrelated dirty work and user changes.
- Do not reformat the entire self-contained HTML.
- Do not replace embedded slide images, base64 assets, or global CSS unless the question requires it.
- When synchronizing variants, insert the same semantic block at the corresponding anchor and verify the new blocks match.

## 7. Verify before finishing

Run the bundled audit against each modified HTML:

```bash
node scripts/audit-qna-html.mjs /absolute/path/to/lecture.html
```

Optionally restrict the report to one new block while still checking global duplicate IDs:

```bash
node scripts/audit-qna-html.mjs /absolute/path/to/lecture.html --id qa-p020-householder-example
```

The audit must report zero errors for:

- duplicate IDs;
- missing question-block IDs;
- student-question blocks with `open`;
- malformed `data-tex` braces or environments;
- naked MathJax delimiters inside question blocks;
- unescaped ampersands inside `data-tex` attributes.

Also perform these checks when the available environment supports them:

- parse the HTML and check inline script syntax;
- open each changed block at desktop and narrow widths;
- confirm formulas render without MathJax `merror`;
- confirm tables and long formulas scroll locally instead of widening the page;
- confirm no text or controls overlap;
- compare synchronized block hashes across variants;
- remove temporary fragments and screenshots.

If local `file://` browser policy blocks visual verification, do not bypass it through another browser surface. Report that limitation and provide the completed static checks.

## Final response

Report the modified HTML path, the question blocks added or revised, whether all student blocks remain default-closed, formula/audit results, synchronized variants, and any verification limitation. Do not claim browser validation if only static checks ran.
