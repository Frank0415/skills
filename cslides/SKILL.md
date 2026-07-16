---
name: cslides
description: Convert one course-slide PDF into a self-contained Chinese HTML study guide. Use for slide-to-HTML work requiring complete page coverage, source images, grouped teaching units, formula and diagram explanations, navigation, MathJax, and visual QA.
---

# cslides

Convert one course-slide PDF into a self-contained Chinese HTML study guide. Reorganize the slides by teaching logic while preserving every original page image and page-level traceability. The result is not a summary, OCR transcript, or one card per page.

## Goal and voice

Build an HTML guide that lets a first-time learner:

- understand the lecture's concepts, formulas, models, methods, examples, and applications;
- follow the knowledge chain between modules and teaching units;
- see why consecutive pages belong together and what task they complete;
- understand formulas, tables, diagrams, circuits, waveforms, and visual progressions;
- receive clearly labeled prerequisite context when the slides omit a necessary bridge;
- search, navigate by page, enlarge slides, print, and read on desktop or mobile.

Write the HTML, navigation, captions, explanations, QA report, and final response in Chinese. Preserve English technical terms, code, APIs, variables, filenames, paths, commands, proper nouns, and source titles when useful, then explain their lecture-specific meaning in Chinese.

Use the `humanizer-zh` style. If that skill is installed, invoke it; otherwise apply these rules directly: write plainly and specifically, omit pleasantries and promotional language, avoid formulaic transitions and forced three-part rhetoric, vary sentence rhythm without sacrificing rigor, and treat the reader as an intelligent beginner. Preserve English terms before their Chinese explanation when that improves precision.

Do not predict exam questions, turn the guide into a question bank, invent unrelated exercises, or add encyclopedia-length background outside the lecture's knowledge chain unless the user asks.

## Completion contract

Do not declare completion until all of the following are true:

1. PDF pages `1...N` were inspected through both extracted text and rendered slide images.
2. Every page belongs to exactly one teaching unit, and units cover only continuous page ranges.
3. Every original slide image appears once, in source order, with its page number.
4. Complete examples, derivations, design flows, circuits, and visual progressions remain intact.
5. Explanations are specific to the source content rather than copied OCR or template language.
6. Visible formulas follow the MathJax contract below and render without errors or naked delimiters.
7. The HTML is self-contained except for the allowed MathJax CDN fallback, opens directly, and has no local absolute asset paths.
8. Search, page location, image zoom, responsive layout, and print styles work.
9. Temporary audit, extraction, contact-sheet, and screenshot-index files are removed unless the user asked to keep them.

If the PDF is encrypted, unreadable, incomplete, or cannot be visually inspected, stop and report the smallest concrete blocker. Do not substitute OCR-only work or claim visual validation that did not happen.

## Single-PDF boundary

One `cslides` execution handles exactly one target PDF. For a directory, archive, or multiple PDFs:

- inventory the PDFs first;
- use `cslides-orchestrator` when it and a working CLI are available;
- otherwise process the PDFs serially as independent single-PDF tasks;
- never pass several PDFs into one conversion prompt;
- never use Python, shell, Node, or another script to loop over PDFs and write multiple HTML files.

Scripts may assist page counting, text extraction, rendering, image embedding, and deterministic validation for the current PDF.

Resolve the target from the explicit path, the newest uploaded PDF when the user clearly means it, or one minimal clarification when genuine ambiguity remains. Record the filename, lecture or chapter title, course, slide-stated author or institution, date when present, and total page count.

## Evidence workflow

### 1. Inspect every page

Extract text from every page to identify titles, definitions, formulas, example conditions, conclusions, table text, captions, terminology, and transitions. Render or open every slide image to verify:

- animation reveals, highlights, and differences between consecutive pages;
- axes, units, curves, intersections, shaded regions, thresholds, and trends;
- circuit elements, polarity, signal direction, switch state, and equivalent models;
- waveform amplitude, phase, period, offset, conduction interval, and clipping;
- flow, causality, and labels in diagrams, tables, structures, and block diagrams;
- superscripts, subscripts, brackets, signs, denominators, limits, and OCR damage in formulas.

Text extraction supplies candidates; the rendered slide is authoritative for formulas and visual meaning. Open dense, image-led, OCR-damaged, example, circuit, waveform, and uncertain-boundary pages at sufficient detail. A contact sheet may check count and order but never replaces page-level inspection.

### 2. Persist a page audit

Before writing the final HTML, create `tmp/<pdf_name>_page_audit.md` or `.txt` on disk. Keep one row per page:

| Page | Screenshot | Text key | Visual evidence | Formula / diagram | Teaching role | Unit | HTML status | Fix notes |
|---|---|---|---|---|---|---|---|---|

Record the page's title or role, important text, visible formula or graphic meaning, animation status, proposed unit, risks, and whether its image is embedded. Use this file to survive long runs and context compaction; do not paste it into the final HTML. Remove it at delivery unless the user requested the evidence.

Subagents may inspect independent semantic page groups within this one PDF. Keep every complete concept, derivation, example, or progression in one subagent's range. Each subagent returns page-level evidence and proposed unit boundaries; the main agent remains responsible for final page continuity, unique assignment, image inclusion, corrected formulas, and cleanup.

### 3. Build the lecture map

Derive the map from all pages, not only the opening agenda:

- lecture overview and central learning problem;
- concepts, formulas or models, analysis methods, and examples or applications actually present;
- modules with number, title, continuous page range, learning task, and transition;
- the lecture's knowledge chain;
- a short index of genuinely central formulas with variables, assumptions, purpose, unit, and pages;
- gaps that need background, derivation entry, model-switch, diagram-reading, or example-prerequisite supplements.

Before authoring HTML, plan each unit with its ID, title, continuous pages, module, type, central question, grouping reason, incoming and outgoing dependency, supplement need, and formula or visual risks.

## Teaching-unit decisions

Merge consecutive pages when they jointly answer one question, complete one derivation or example, analyze one model or circuit, evolve one diagram or waveform, or reveal one slide through animation. Split when a new topic, example, model, assumption, operating region, or substantial application begins. A normal long-deck unit often covers 2-6 pages; use whatever range preserves the real teaching task.

Never merge non-adjacent pages into one body unit. Cross-unit comparisons belong in overview or summary sections without changing source order.

Keep every source example as an `Example Unit` containing its problem, known quantities, target, method, intermediate steps, result, and interpretation. Do not split the statement from its solution or replace the worked process with a generic summary.

Distinguish evidence levels:

- `课件原意`: directly stated by source text, formula, table, or image;
- `合理解读`: supported by visible evidence and adjacent context;
- `上下文补足`: standard prerequisite or bridge absent from the slides.

Label supplements as background, derivation entry, model switch, diagram reading, example prerequisite, or inserted topic. Do not present added context as slide content or invent teacher intent. Prefer phrasing such as “从知识组织逻辑看” and “图中可以读出” over unsupported certainty.

## Writing each unit

Use concise but complete bullet points rather than long essay paragraphs. Adapt the bullets to the content; do not fill a template mechanically. Explain:

- the question the unit answers;
- concepts, variables, assumptions, and mechanisms;
- the actual derivation, analysis, comparison, or example sequence;
- how to read each important graph, table, circuit, waveform, or diagram;
- the conclusion and the ability the learner should gain.

Write concrete transitions: what the previous unit established, what this unit adds, how pages inside the unit progress, and why the next unit follows. For an inserted application, state that it is not the mathematical next step and explain its real link to the main line.

Do not create a verbose “每页贡献” or OCR-excerpt block. Page ranges, the slide gallery, page index, and internal audit provide traceability.

Each unit uses this semantic structure; omit optional elements when the source does not need them:

```html
<article class="unit-card" id="unit-U01" data-pages="12 13 14">
  <div class="card-head">
    <div class="unit-meta">
      <span class="unit-badge">U01 · 概念 / 推导 / EXAMPLE / 模型 / 应用</span>
      <span class="page-range">P012-P014 / TOTAL</span>
    </div>
    <h3>讲解单元标题</h3>
    <span class="context-pill">含上下文补足</span>
  </div>
  <p class="group-rationale"><strong>为什么合在一起讲:</strong> 具体的教学理由。</p>
  <div class="slide-gallery">
    <figure class="slide-shot" data-page="12">
      <img src="data:image/webp;base64,..." alt="原课件第 12 页" />
      <figcaption>
        <span>P012 · 原始标题或简短作用说明</span>
        <button class="zoom-btn" type="button">放大查看</button>
      </figcaption>
    </figure>
  </div>
  <div class="commentary">
    <section class="unit-explanation">
      <h4>完整讲解</h4>
      <ul class="teaching-points">
        <li><strong>本单元要回答的问题:</strong> ...</li>
        <li><strong>核心概念:</strong> ...</li>
        <li><strong>关键步骤:</strong> ...</li>
        <li><strong>图 / 表 / 电路 / 波形的读取方式:</strong> ...</li>
        <li><strong>本单元结论:</strong> ...</li>
      </ul>
      <div class="math-display" data-tex="\left\langle d_i, d_j \right\rangle">
        \left\langle d_i, d_j \right\rangle
      </div>
    </section>
    <section class="transition-map">
      <h4>本单元在知识链中的位置</h4>
      <div class="transition-grid">
        <div class="transition-item incoming"><strong>承接上一单元</strong><p>...</p></div>
        <div class="transition-item role"><strong>本单元任务</strong><p>...</p></div>
        <div class="transition-item within"><strong>组内推进</strong><p>...</p></div>
        <div class="transition-item outgoing"><strong>导向下一单元</strong><p>...</p></div>
      </div>
    </section>
    <details class="supplement">
      <summary>上下文补足:补充主题 <span class="supplement-kind">补足类型</span></summary>
      <div class="supplement-body"><ul><li>必要且直接相关的补充。</li></ul></div>
    </details>
    <div class="mastery"><strong>读完本单元应掌握:</strong> 一至三句具体能力。</div>
  </div>
</article>
```

## HTML information architecture

Keep this order:

1. **Hero**: course or chapter label, `[Lecture 标题] | 内容聚合讲解（逻辑增强版）`, one-line knowledge-chain subtitle, source PDF, page count, module count, unit count, merged-unit count, Example count, and a brief page-audit statement.
2. **Sticky navigation**: search, module links, unit links with type and pages, and complete `P001...PN` page location. On mobile, collapse it into a usable top control.
3. **Four overview panels**:
   - what the slides teach, their central question, categories, and target ability;
   - the lecture's knowledge flow, main line, examples, applications, and real jumps;
   - a unit route table with ID, title, pages, type, grouping reason, supplement units, Example units, and visual-focus units;
   - the central formula index with variables, conditions, purpose, unit, and source pages.
4. **Module sections**: number, title, pages, task, transition, then ordered unit cards.
5. **Footer**: source PDF, counts, supplement count, coverage statement, and `内容聚合讲解 · 逻辑增强版`.

Search must cover IDs, page numbers (`23`, `P23`, `P023`), titles, modules, source captions, prose, and formula keywords. Page location scrolls to the owning unit and briefly highlights it. Image zoom opens a full-screen overlay, shows the page number, closes by button or Esc, and preferably supports previous or next slide within a unit.

## Slide images and visual explanation

Render every page clearly, prefer WebP, and embed it as a base64 data URI. Use accurate alt text and page captions. Thumbnails must show structure; zoomed images must make formulas and annotations readable. Keep gallery images in page order, choose one to three columns responsively, and never hide animation or transition pages.

For each meaningful visual, explain what it shows, where to look first, what axes, units, colors, directions, curves, parameters, or components mean, and how the visual supports the conclusion. When an image is merely illustrative, say so instead of inferring unsupported quantitative relations.

## Mathematics contract

Reconstruct formulas from the slide image, not broken OCR. Verify superscripts, subscripts, brackets, limits, signs, denominators, units, and whether letters are variables or labels. Explain variables, use, and assumptions.

Put every visible mathematical expression through one rendering path:

```html
<span class="math-text" data-tex="X^\top X">X^\top X</span>
<div class="math-display" data-tex="\left\langle d_i, d_j \right\rangle">
  \left\langle d_i, d_j \right\rangle
</div>
```

Requirements:

- keep fallback text semantically identical to `data-tex`;
- escape attribute-level `&`, `<`, `>`, and quotes as HTML entities; matrix separators therefore use `&amp;` in source;
- never leave visible `\(...\)` or `\[...\]` delimiters;
- wrap short math in `.math-text` and standalone derivations in `.math-display`;
- use standard TeX such as `\mathrm{}`, `\text{}`, `\frac`, `\ln`, `\parallel`, `\int`, `\frac{\mathrm d}{\mathrm dt}`, `\partial`, `\rightarrow`, and `\mapsto`;
- write prime as `X^{\prime}`, transpose as `X^\top`, and angle brackets as `\left\langle ... \right\rangle`;
- keep HTML tags out of `alt`, `title`, and `aria-label`; use plain text there and render math separately in visible captions;
- escape backslashes again only when TeX is embedded inside a JavaScript, Python, or JSON string.

Load MathJax 3 and render the `data-tex` elements:

```html
<script>
window.MathJax = {
  tex: { inlineMath: [['\\(','\\)']], displayMath: [['\\[','\\]']], processEscapes: true },
  svg: { fontCache: 'global' }
};
</script>
<script defer src="https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-svg.js"></script>
<script>
window.addEventListener('load', async () => {
  if (!window.MathJax?.tex2svgPromise) return;
  for (const el of document.querySelectorAll('.math-text[data-tex], .math-display[data-tex]')) {
    const tex = el.dataset.tex || el.textContent.trim();
    const svg = await MathJax.tex2svgPromise(tex, { display: el.classList.contains('math-display') });
    el.replaceChildren(svg);
  }
});
</script>
```

Inspect all formula locations, including headings, navigation, tables, captions, formula index, examples, and supplements. Check for unmatched braces or environments, lost backslashes, raw `X^T X`, `A^T A`, `X&#x27;`, `X\&#x27;`, `\night`, `\nangle`, red MathJax text, `Extra \left`, and unrecognized delimiters. Confirm the formula index, at least one dense formula unit, and at least one Example formula render correctly.

If external MathJax cannot load, use an available local renderer or pre-render formulas as embedded SVG or MathML. Never deliver a page with naked failed TeX; report the actual rendering route.

## Visual and technical design

Use this restrained course-guide theme:

```css
:root {
  --navy: #063e88;
  --blue: #0b5bb4;
  --sky: #e7f0fb;
  --ink: #16263b;
  --muted: #5b6d83;
  --paper: #ffffff;
  --bg: #f3f6fb;
  --line: #dce5f1;
  --green: #096c67;
  --orange: #c75c14;
  --shadow: 0 12px 28px rgba(16,40,72,.08);
  --radius: 18px;
}
```

- Use a blue Hero gradient, serif display headings, and Chinese-capable sans-serif body text.
- Keep the overall width near `1320px` and desktop navigation near `290px`.
- Use white panels, fine borders, restrained shadows, and the defined radius.
- Use blue for structure, green for conclusions and conditions, orange for supplements and model switches, pale blue for knowledge flow, and blue-green labels for Examples.
- Keep galleries and commentary spacious on desktop and single-column on narrow screens.
- Prevent text, controls, tables, and formulas from overlapping or widening the page; allow local horizontal scrolling for wide formulas and tables.

Use semantic `header`, `nav`, `main`, `section`, `article`, `figure`, `figcaption`, `details`, and `footer` elements. Give each unit a unique ID, `data-pages`, searchable text, and page mapping. Inline CSS and JavaScript. Do not depend on local absolute paths.

Print CSS hides navigation, search, zoom controls, overlays, and other interactive controls while preserving the Hero, overview panels, formulas, module headings, unit explanations, slide images, transitions, and mastery summaries. Avoid splitting a unit heading from its content and keep A4 output legible.

## Verification

Before delivery:

1. Compare PDF page count, audit rows, `data-page` values, embedded images, and the page index; require exactly `1...N`, no duplicates, and source order.
2. Review unit boundaries for split examples, derivations, models, and visual progressions; remove generic or repeated explanations.
3. Confirm every key formula and visual is interpreted, every supplement is labeled, and no unsupported teacher intent or factual invention appears.
4. Parse the HTML, check unique IDs and inline script syntax, and scan for placeholders, malformed tags, local paths, naked TeX, and MathJax errors.
5. Open the artifact and inspect desktop and narrow layouts, search, page location, zoom, responsive overflow, and print behavior. Do not claim browser validation if only static checks ran.
6. Delete temporary work files after all checks pass.

User-added constraints may change emphasis, language, terminology, print preferences, supplement defaults, and level of detail. They do not override page coverage, source order, complete source examples, mathematical correctness, single-file delivery, or evidence-based validation unless the user explicitly changes that invariant.

## Final response

Keep the chat response concise. Report:

- generated HTML path or clickable link and source PDF;
- PDF pages, modules, units, merged units, Example units, and supplement units;
- page-coverage, formula-rendering, page-level visual inspection, and temporary-file cleanup results;
- any verification limitation or known quality caveat.

Do not paste the HTML into chat.
