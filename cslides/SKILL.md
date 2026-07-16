---
name: cslides
description: Convert one course-slide PDF into a self-contained Chinese HTML study guide. Use for slide-to-HTML work requiring complete page coverage, source images, grouped teaching units, formula and diagram explanations, navigation, MathJax, and visual QA.
---

# cslides

Convert one course-slide PDF into a self-contained Chinese HTML study guide. Reorganize the slides by teaching logic while preserving every original page image and page-level traceability. The result is not a summary, OCR transcript, or one card per page.

## Required output template

Before authoring HTML, read `assets/cslides-template.html` relative to this `SKILL.md` and use it as the output shell. Copy its complete document structure, `#cslides-theme` stylesheet, navigation controls, lightbox, MathJax setup, print behavior, and interaction script into the generated file. Then:

- replace every `{{PLACEHOLDER}}` with lecture-specific content or remove the optional element;
- expand the representative module, unit, slide, and page-detail blocks to cover the real PDF;
- preserve the template's class names, element IDs, design tokens, component hierarchy, and JavaScript hooks;
- do not invent a new theme, substitute another generic dashboard layout, or delete an interaction because the deck is long;
- add source-specific markup or CSS only when the lecture genuinely needs a component the template does not provide, and build it from the existing tokens and density scale.

The template is the consistency contract across independent agents and many PDFs. If the `apple-design` skill is available, use it as a review lens for hierarchy, legibility, spatial consistency, immediate feedback, and accessibility; the fixed cslides template remains authoritative. If the template asset is missing or unreadable, report the broken skill installation instead of silently producing an unrelated layout.

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
5. Every page has exactly one page-detail block with a complete line-by-line Chinese translation of the source page and a page-specific explanation.
6. Each unit's always-visible explanation is detailed enough to teach the unit without opening any folded section.
7. Explanations are specific to the source content rather than copied OCR or template language.
8. Visible formulas follow the MathJax contract below and render without errors or naked delimiters.
9. The HTML is self-contained except for the allowed MathJax CDN fallback, opens directly, and has no local absolute asset paths.
10. Search, page location, image zoom, responsive layout, and print styles work.
11. Temporary audit, extraction, contact-sheet, and screenshot-index files are removed unless the user asked to keep them.

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

| Page | Screenshot | Text key | Visual evidence | Formula / diagram | Teaching role | Unit | Image status | Translation status | Explanation status | Fix notes |
|---|---|---|---|---|---|---|---|---|---|---|

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

Each unit has two complementary teaching layers. Do not make the learner open a disclosure to receive the real explanation.

### Always-visible complete explanation

Write a dense, source-specific explanation in `.unit-explanation`. It must stand alone as the main lesson, not act as a teaser for the folded page notes. Teach all slides in the unit as one integrated narrative: combine their definitions, visual evidence, formulas, intermediate reasoning, and conclusions into a coherent explanation of the shared teaching task. Do not write one shallow bullet per page or simply concatenate page summaries.

Write connected prose under source-specific subsection headings. Bullets are appropriate only for a real sequence, comparison set, or list of parallel conditions; they must not be the default shape of the lesson. The following are coverage questions, not five template labels:

- the concrete question the unit answers and why it matters here;
- definitions, variables, assumptions, units, and mechanisms;
- the actual derivation, algorithm, comparison, design flow, or worked-example sequence;
- intermediate reasoning that the slides compress or leave implicit;
- how to read each important graph, table, circuit, waveform, diagram, or animation progression;
- the meaning of the result, its conditions or limitations, and the ability the learner should gain;
- the dependency on the previous unit and the reason the next unit follows.

Do not satisfy these items with generic labels followed by one vague sentence. Name the source-specific quantities, operations, visual evidence, and conclusions. Preserve the full problem-to-result chain for examples and derivations.

For dense material, put the full teaching depth here:

- for a formula or derivation, define the target quantity and variables, reconstruct the missing intermediate reasoning, explain the result's meaning, and state its assumptions;
- for a convergence or complexity claim, define the error or cost being measured, explain where each factor comes from, distinguish parallel work from sequential dependency, and state when the rate does not apply;
- for an algorithm, explain state, inputs, update order, synchronization or data dependencies, stopping condition, and the tradeoff that motivates the design;
- for an example, carry the source values through the method to the result and interpret the result in context;
- for a graph, table, circuit, waveform, or diagram, explain how to read the visual evidence before giving the conclusion;
- add a small numerical or conceptual example when it materially clarifies an abstract rate, bound, or update rule, while labeling it as teaching context rather than slide content.

Use as many paragraphs, equations, or substeps as the learner needs. For a multi-slide unit, trace the causal progression across the slides while explaining the shared idea; do not create isolated mini-summaries for P012, P013, and P014. Do not reduce a derivation to a result, an algorithm to a list of nouns, or a complexity claim to the final big-O expression. The visible explanation should make clear how the pages work together, why the steps are valid, and under which conditions the conclusion holds.

A multi-slide unit is not complete if its main lesson is only five one-sentence checklist items. Hide the gallery and all disclosures, then ask whether an unfamiliar learner could reconstruct the setup, the cross-slide mechanism or derivation, the role of the formulas or visuals, the conclusion, and its limits from `.unit-explanation` alone. If not, deepen the integrated explanation before continuing.

### Folded page-by-page explanation

After the always-visible explanation and transition map, add one mandatory `details.page-notes` section for the unit. It contains exactly one `.page-explain[data-page-detail]` block for every page in the unit, in source order. Across the document, `data-page-detail` values must be exactly `1...N`, with no gaps or duplicates.

Every page block contains:

1. `原页逐句翻译`: a complete Chinese translation of the source page, in source semantic order but rewritten as natural continuous Chinese prose.
2. `本页解释`: a page-specific explanation of terminology, formulas, visuals, assumptions, and the page's role in the unit.

The translation is a source-fidelity layer, not a summary or loose paraphrase. Translate every visible title, subtitle, sentence, bullet, nested bullet, table cell, axis label, legend, caption, callout, annotation, and code comment that carries meaning. Preserve formulas, variables, code, proper nouns, and useful English technical terms exactly while translating their surrounding prose. Keep the source's semantic order and every claim, but do not mechanically reproduce the slide's bullets, columns, or table layout. Join related source lines into one natural paragraph with commas, semicolons, or short connective phrases; retain a table or list only when converting it to prose would destroy a real comparison or sequence. Do not omit repeated text on an animation page or add teaching claims inside the translation. Reconstruct text from the rendered page rather than copying damaged OCR. If a fragment remains unreadable after visual inspection, mark the smallest fragment as `原页文字不清` instead of guessing.

Place the complete translation and `本页解释` in one `.page-explain-body` frame, with the explanation immediately following the translation. Do not split them into columns, cards, or separately bordered regions. In the explanation, clarify the page's terms, formula notation, diagram-reading order, assumptions, caveats, corrections, and contribution to the unit. For animation or progressive-reveal pages, state what changed from the preceding page and why it matters. Title, agenda, divider, recap, and closing pages may need only a short explanation, but their translation must still be complete.

Keep cross-page synthesis and the full derivation or algorithm walkthrough in the always-visible `完整讲解`; do not bury the real lesson in page notes or duplicate the same long explanation on several pages. The page-specific explanation may be detailed, but it stays anchored to what this page contributes.

Finish each `details.page-notes` section with a concise `本单元页间主线` that connects the pages without repeating the always-visible explanation. Reconstruct the translation from source evidence rather than pasting raw OCR, and do not duplicate the same explanatory paragraph across pages.

Write concrete transitions: what the previous unit established, what this unit adds, how pages inside the unit progress, and why the next unit follows. For an inserted application, state that it is not the mathematical next step and explain its real link to the main line.

Each unit uses this semantic structure. The page-notes block is required; omit only the source-dependent optional elements:

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
      <div class="explanation-flow">
        <section class="explanation-section">
          <h5>问题如何建立</h5>
          <div class="explanation-body"><p>融合本组 slides 的起点、变量、条件和共同问题。</p><p>解释为什么需要进入后续机制。</p></div>
        </section>
        <section class="explanation-section">
          <h5>机制与推导</h5>
          <div class="explanation-body"><p>按因果关系讲清跨页机制、推导或 example 全过程。</p><p>解释每一步为什么成立，并在需要处嵌入公式或图像解读。</p></div>
        </section>
        <section class="explanation-section">
          <h5>结果、边界与连接</h5>
          <div class="explanation-body"><p>解释结论、限制、工程含义及它如何导向下一单元。</p></div>
        </section>
      </div>
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
    <details class="page-notes" id="unit-U01-pages">
      <summary>
        <span><strong>逐页详解</strong><small>P012-P014 · 3 页</small></span>
        <span class="detail-purpose">逐句翻译 · 本页解释</span>
      </summary>
      <div class="page-notes-body">
        <section class="page-explain" data-page-detail="12">
          <header><span class="page-chip">P012</span><h5>原始标题或页面作用</h5></header>
          <div class="page-explain-body">
            <p class="page-translation" data-page-translation="12"><strong>原页逐句翻译:</strong> 不遗漏原页内容，按语义顺序改写成自然连续的中文。</p>
            <p class="page-interpretation"><strong>本页解释:</strong> 紧接翻译，解释本页术语、公式、视觉信息及其在单元中的作用。</p>
          </div>
        </section>
      </div>
      <p class="page-thread"><strong>本单元页间主线:</strong> P012 如何推进到 P014。</p>
    </details>
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
4. **Module sections**: number, title, pages, task, transition, then ordered unit cards. Each unit includes the always-visible complete explanation and the mandatory folded page-by-page explanation.
5. **Footer**: source PDF, counts, supplement count, coverage statement, and `内容聚合讲解 · 逻辑增强版`.

Search must cover IDs, page numbers (`23`, `P23`, `P023`), titles, modules, source captions, prose, and formula keywords. Page location scrolls to the owning unit and briefly highlights it. Image zoom opens a full-screen overlay, shows the page number, closes by button or Esc, and preferably supports previous or next slide within a unit.

## Slide images and visual explanation

Render every page clearly, prefer WebP, and embed it as a base64 data URI. Use accurate alt text and page captions. Thumbnails must show structure; zoomed images must make formulas and annotations readable. Keep gallery images in page order. Let the fixed template choose the number of side-by-side slides from the available HTML width instead of forcing two or three columns. During screen reading, the visible gallery area stays near one-third of the viewport height and scrolls locally when additional rows are necessary; print shows every slide without that height cap. Never remove animation or transition pages.

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

Treat information density as the first visual criterion: the learner should see the lecture structure, source slides, and substantive explanation without oversized decoration or excessive empty space. Use the fixed template rather than rewriting its CSS. Its visual system intentionally provides:

- a neutral canvas, bright reading surfaces, strong black text, and distinct blue, green, amber, and violet semantic accents rather than a one-color theme;
- system sans-serif typography with hierarchy from size, weight, spacing, and grouping rather than decorative display fonts;
- compact translucent navigation, direct controls, visible focus states, immediate press feedback, and restrained motion;
- light, neutral slide galleries whose `auto-fit` columns respond to the actual content width, whose visible area does not exceed roughly one-third of the viewport height, and whose full-size lightbox preserves legibility;
- one framed repeated item per teaching unit, with internal sections separated by rhythm and rules rather than nested card styling;
- clear visual distinction among always-visible teaching, folded page notes, optional context supplements, formulas, transitions, and mastery outcomes;
- reduced-motion, reduced-transparency, increased-contrast, mobile navigation, overflow, and print handling.

Do not enlarge headings or spacing merely to make the page feel more designed. Do not add gradients, decorative blobs, stock imagery, floating section cards, negative letter spacing, or animation unrelated to navigation and feedback. Prevent text, controls, tables, formulas, and slide captions from overlapping or widening the page; use local horizontal scrolling only where the content is intrinsically wide.

Use semantic `header`, `nav`, `main`, `section`, `article`, `figure`, `figcaption`, `details`, and `footer` elements. Give each unit a unique ID, `data-pages`, searchable text, and page mapping. Inline CSS and JavaScript. Do not depend on local absolute paths. Keep the template's interaction IDs unique and do not rename them without updating every matching script and accessibility reference.

Print CSS hides navigation, search, zoom controls, overlays, and other interactive controls while preserving the Hero, overview panels, formulas, module headings, unit explanations, slide images, transitions, and mastery summaries. Avoid splitting a unit heading from its content and keep A4 output legible.

## Verification

Before delivery:

1. Compare PDF page count, audit rows, `data-page` values, embedded images, and the page index; require exactly `1...N`, no duplicates, and source order.
2. Compare PDF page count with `.page-explain[data-page-detail]` and `.page-translation[data-page-translation]`; require exactly one of each for every page `1...N`, in order. Compare every source page against its translation and confirm no meaningful source line, label, bullet, table cell, caption, or annotation was summarized away or omitted.
3. Review unit boundaries for split examples, derivations, models, and visual progressions; remove generic or repeated explanations. Read each always-visible explanation with all disclosures closed and confirm it integrates the grouped slides into a complete lesson. Reject multi-slide units reduced to five one-sentence checklist items. Sample at least one dense formula or complexity unit, one algorithm or example unit, and one visual unit; confirm the full reasoning chain appears in `完整讲解` rather than being deferred to page notes.
4. Confirm every key formula and visual is interpreted, every supplement is labeled, and no unsupported teacher intent or factual invention appears.
5. Parse the HTML, check unique IDs and inline script syntax, and scan for unresolved `{{...}}` placeholders, malformed tags, local paths, naked TeX, and MathJax errors.
6. Open the artifact in a real browser and inspect desktop and narrow layouts with all disclosures closed first, then with representative page notes and supplements open. Check information density, hierarchy, clipping, overlap, search, page location, zoom, responsive overflow, and print behavior. When computer-use is available, inspect screenshots directly. Do not claim browser validation if only static checks ran.
7. Delete temporary work files after all checks pass.

User-added constraints may change emphasis, language, terminology, print preferences, supplement defaults, and level of detail. They do not override page coverage, source order, complete source examples, mathematical correctness, single-file delivery, or evidence-based validation unless the user explicitly changes that invariant.

## Final response

Keep the chat response concise. Report:

- generated HTML path or clickable link and source PDF;
- PDF pages, modules, units, merged units, Example units, supplement units, and page-detail blocks;
- page-coverage, formula-rendering, page-level visual inspection, and temporary-file cleanup results;
- any verification limitation or known quality caveat.

Do not paste the HTML into chat.
