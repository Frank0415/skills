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

Apply the complete embedded `humanizer-zh` editing guide while drafting and during the final prose review. It is part of this skill and has no external skill dependency. Preserve source fidelity, mathematical correctness, the course's technical register, and the cslides output contract when applying its general writing advice.

Do not predict exam questions, turn the guide into a question bank, invent unrelated exercises, or add encyclopedia-length background outside the lecture's knowledge chain unless the user asks.

## Completion contract

Do not declare completion until all of the following are true:

1. PDF pages `1...N` were inspected through both extracted text and rendered slide images.
2. Every page belongs to exactly one teaching unit, and units cover only continuous page ranges.
3. Every original slide image appears once, in source order, with its page number.
4. Complete examples, derivations, design flows, circuits, and visual progressions remain intact.
5. Every page has exactly one page-detail block with a complete Chinese translation of its instructional content and a page-specific explanation.
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

Write a source-specific explanation in `.unit-explanation`. It must stand alone as the main lesson, not act as a teaser for the folded page notes. Teach all slides in the unit as one integrated narrative: combine the definitions, visual evidence, formulas, reasoning, and conclusions needed to explain the shared teaching task. Do not write one shallow bullet per page or simply concatenate page summaries.

Before drafting that narrative, derive a private coverage ledger from the unit's page-audit rows. Record the substantive definitions, claims, formulas, algorithm steps, comparisons, visual cues, assumptions, caveats, and source conclusions. Explain each relevant item once where it advances the shared argument. Do not print the ledger in the HTML. Page notes preserve source wording page by page; `.unit-explanation` interprets the evidence, supplies necessary missing reasoning, and connects it across pages.

Write connected prose under source-specific subsection headings. Bullets are appropriate only for a real sequence, comparison set, or list of parallel conditions; they must not be the default shape of the lesson. The following are coverage questions, not five template labels:

- the concrete question the unit answers and why it matters here;
- definitions, variables, assumptions, units, and mechanisms;
- the actual derivation, algorithm, comparison, design flow, or worked-example sequence;
- intermediate reasoning that the slides compress or leave implicit;
- how to read each important graph, table, circuit, waveform, diagram, or animation progression;
- the meaning of the result, its conditions or limitations, and the ability the learner should gain;
- the dependency on the previous unit and the reason the next unit follows.

Do not satisfy these items with generic labels followed by one vague sentence. Name the source-specific quantities, operations, visual evidence, and conclusions. Preserve the full problem-to-result chain for examples and derivations.

Lead each substantive technical unit with one short `.core-idea` block that states the central idea, relation, or decision rule in one to three sentences. Identify the one or two formulas that carry the unit's reasoning and render them near the explanation they support with `.math-display`; keep secondary notation inline. The reader should be able to scan the core idea, key formulas, and subsection headings to recover the knowledge spine before reading the details. Do not promote every definition, caveat, or intermediate identity to equal visual weight.

For material that needs unpacking, put the cross-page teaching here:

- for a formula or derivation, define the target quantity and variables, reconstruct the missing intermediate reasoning, explain the result's meaning, and state its assumptions;
- for a convergence or complexity claim, define the error or cost being measured, explain where each factor comes from, distinguish parallel work from sequential dependency, and state when the rate does not apply;
- for an algorithm, explain state, inputs, update order, synchronization or data dependencies, stopping condition, and the tradeoff that motivates the design;
- for an example, carry the source values through the method to the result and interpret the result in context;
- for a graph, table, circuit, waveform, or diagram, explain how to read the visual evidence before giving the conclusion;
- add a small numerical or conceptual example when it materially clarifies an abstract rate, bound, or update rule, while labeling it as teaching context rather than slide content.

Use only the paragraphs, equations, and substeps needed to teach the material clearly. For a multi-slide unit, trace the causal progression across the slides while explaining the shared idea; do not create isolated mini-summaries for P012, P013, and P014. Do not reduce a derivation to a result, an algorithm to a list of nouns, or a complexity claim to the final big-O expression. The visible explanation should make clear how the pages work together, why the steps are valid, and under which conditions the conclusion holds.

Match explanation length to the unit's teaching density. A divider, recap, repeated animation, or straightforward definition should stay short; a real derivation, algorithm, comparison, or worked Example may need more room. Reopen the coverage ledger only when a necessary definition, reasoning step, formula interpretation, visual cue, tradeoff, or limit is missing. Add missing teaching content, but remove padding, repeated translation, duplicated conclusions, and artificial subsectioning. Each paragraph should establish a concrete claim and the reason or implication that helps the learner understand it.

Describe evidence positively: state what a graph, formula, circuit, or source passage shows and how to read it. Do not pad the lesson with statements about what cannot be inferred, guessed, generalized, or numerically recovered; omit those sentences.

A multi-slide unit is not complete if its main lesson is only a terse checklist. Hide the gallery and all disclosures, then ask whether an unfamiliar learner could reconstruct the setup, the cross-slide mechanism or derivation, the role of the formulas or visuals, the conclusion, and its limits from `.unit-explanation` alone. If not, add the missing explanation before continuing.

### Folded page-by-page explanation

After the always-visible explanation and transition map, add one mandatory `details.page-notes` section for the unit. It contains exactly one `.page-explain[data-page-detail]` block for every page in the unit, in source order. Across the document, `data-page-detail` values must be exactly `1...N`, with no gaps or duplicates.

Every page block contains:

1. `原页逐句翻译`: a complete Chinese translation of the page's instructional content, in source semantic order but rewritten as natural continuous Chinese prose.
2. `本页解释`: a concise page-specific explanation of the points that need clarification and the page's role in the unit.

The translation is a source-fidelity layer for the lesson, not a summary or loose paraphrase. Translate every instructional title, subtitle, sentence, bullet, nested bullet, table cell, axis label, legend, caption, callout, annotation, and code comment that carries course meaning. Exclude repeated slide-master chrome such as section-navigation tabs, progress markers, institutional branding, course footers, lecture labels, and page numbers unless the slide itself discusses them or they are necessary to understand the lesson. Do not narrate the layout with phrases such as `页眉导航依次为`, `左图标为`, `标题为`, or `页脚为`; start directly with the translated idea and integrate labels where they explain the content. Preserve formulas, variables, code, proper nouns, and useful English technical terms exactly while translating their surrounding prose. Keep the instructional content's semantic order and every teaching claim, but do not mechanically reproduce the slide's bullets, columns, or table layout. Join related source lines into natural prose with commas, semicolons, or short connective phrases; retain a table or list only when converting it to prose would destroy a real comparison or sequence. Do not repeat unchanged text on an animation page when it adds no new teaching content; translate the new or changed content and state the progression. Reconstruct text from the rendered page rather than copying damaged OCR. If an instructional fragment remains unreadable after visual inspection, mark the smallest fragment as `原页文字不清` instead of guessing.

Place the complete translation and `本页解释` in one `.page-explain-body` frame, with the explanation immediately following the translation. Do not split them into columns, cards, or separately bordered regions. Clarify only what this page actually needs, such as unfamiliar terms, formula notation, diagram-reading order, assumptions, or its contribution to the unit. Do not restate points already taught clearly in `完整讲解`. For animation or progressive-reveal pages, state what changed from the preceding page and why it matters. Title, agenda, divider, recap, and closing pages usually need only a short explanation of their instructional content.

Keep cross-page synthesis and the derivation or algorithm walkthrough in the always-visible `完整讲解`; do not bury the real lesson in page notes or duplicate it across pages. Keep each page-specific explanation anchored to what that page uniquely contributes.

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
      <div class="core-idea"><strong>核心思想:</strong> 用一至三句写出本单元最重要的关系、机制或判断规则。</div>
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
            <p class="page-translation" data-page-translation="12"><strong>原页逐句翻译:</strong> 不遗漏原页教学内容，省略重复母版元素，按语义顺序改写成自然连续的中文。</p>
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

Render every page clearly, prefer WebP, and embed it as a base64 data URI. Use accurate alt text and page captions. Thumbnails must show structure; zoomed images must make formulas and annotations readable. Keep gallery images in page order. The fixed template uses at most three side-by-side slides: three columns on wide unit cards, two on medium cards, and one on narrow cards. Do not introduce a fourth column or cap each slide to an artificially narrow fixed width. Let the gallery expand to show every slide and caption in full, including additional rows. Never add an internal vertical scrollbar or clipped gallery viewport; the document itself is the only vertical scrolling surface. Never remove animation or transition pages.

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
- light, neutral slide galleries whose container-aware `3/2/1` columns respond to the unit-card width, never exceed three columns, keep each thumbnail useful for reading, and expand to show every slide and caption without an internal scrollbar; the full-size lightbox preserves full-detail legibility;
- one framed repeated item per teaching unit, with internal sections separated by rhythm and rules rather than nested card styling;
- clear visual distinction among always-visible teaching, folded page notes, optional context supplements, formulas, transitions, and mastery outcomes;
- reduced-motion, reduced-transparency, increased-contrast, mobile navigation, overflow, and print handling.

Do not enlarge headings or spacing merely to make the page feel more designed. Do not add gradients, decorative blobs, stock imagery, floating section cards, negative letter spacing, or animation unrelated to navigation and feedback. Prevent text, controls, tables, formulas, and slide captions from overlapping or widening the page; use local horizontal scrolling only where the content is intrinsically wide.

Use semantic `header`, `nav`, `main`, `section`, `article`, `figure`, `figcaption`, `details`, and `footer` elements. Give each unit a unique ID, `data-pages`, searchable text, and page mapping. Inline CSS and JavaScript. Do not depend on local absolute paths. Keep the template's interaction IDs unique and do not rename them without updating every matching script and accessibility reference.

Print CSS hides navigation, search, zoom controls, overlays, and other interactive controls while preserving the Hero, overview panels, formulas, module headings, unit explanations, slide images, transitions, and mastery summaries. Avoid splitting a unit heading from its content and keep A4 output legible.

## Verification

Before delivery:

1. Compare PDF page count, audit rows, `data-page` values, embedded images, and the page index; require exactly `1...N`, no duplicates, and source order.
2. Compare PDF page count with `.page-explain[data-page-detail]` and `.page-translation[data-page-translation]`; require exactly one of each for every page `1...N`, in order. Compare every source page against its translation and confirm no instructional line, label, bullet, table cell, caption, or annotation was summarized away or omitted. Confirm repeated navigation tabs, branding, course footers, and page numbers were not copied into the translation.
3. Review unit boundaries for split examples, derivations, models, and visual progressions; remove generic, repeated, or padded explanations. Read each always-visible explanation with all disclosures closed and confirm it integrates the grouped slides into a coherent lesson. Reject checklist-only summaries, but do not expand a unit merely to meet a length target. Sample at least one formula or complexity unit, one algorithm or example unit, and one visual unit; confirm the necessary reasoning chain appears in `完整讲解` rather than being deferred to page notes.
4. Confirm every key formula and visual is interpreted, every supplement is labeled, and no unsupported teacher intent or factual invention appears.
5. Parse the HTML, check unique IDs and inline script syntax, and scan for unresolved `{{...}}` placeholders, malformed tags, local paths, naked TeX, and MathJax errors.
6. Open the artifact in a real browser and inspect desktop and narrow layouts with all disclosures closed first, then with representative page notes and supplements open. Check information density, hierarchy, clipping, overlap, search, page location, zoom, responsive overflow, and print behavior. When computer-use is available, inspect screenshots directly. Do not claim browser validation if only static checks ran.
7. Delete temporary work files after all checks pass.

User-added constraints may change emphasis, language, terminology, print preferences, supplement defaults, and level of detail. They do not override page coverage, source order, complete source examples, mathematical correctness, single-file delivery, or evidence-based validation unless the user explicitly changes that invariant.

## Embedded humanizer-zh editing guide

Apply every relevant detection and rewriting rule below to the Chinese teaching prose. This embedded copy is complete; do not substitute a shorter summary.


### Humanizer-zh: 去除 AI 写作痕迹

你是一位文字编辑，专门识别和去除 AI 生成文本的痕迹，使文字听起来更自然、更有人味。本指南基于维基百科的"AI 写作特征"页面，由 WikiProject AI Cleanup 维护。

#### 你的任务

当收到需要人性化处理的文本时：

1. **识别 AI 模式** - 扫描下面列出的模式
2. **重写问题片段** - 用自然的替代方案替换 AI 痕迹
3. **保留含义** - 保持核心信息完整
4. **维持语调** - 匹配预期的语气（正式、随意、技术等）
5. **注入灵魂** - 不仅要去除不良模式，还要注入真实的个性

---

#### 核心规则速查

在处理文本时，牢记这 5 条核心原则：

1. **删除填充短语** - 去除开场白和强调性拐杖词
2. **打破公式结构** - 避免二元对比、戏剧性分段、修辞性设置
3. **变化节奏** - 混合句子长度。两项优于三项。段落结尾要多样化
4. **信任读者** - 直接陈述事实，跳过软化、辩解和手把手引导
5. **删除金句** - 如果听起来像可引用的语句，重写它

---

#### 个性与灵魂

避免 AI 模式只是工作的一半。无菌、没有声音的写作和机器生成的内容一样明显。好的写作背后有一个真实的人。

##### 缺乏灵魂的写作迹象（即使技术上"干净"）：
- 每个句子长度和结构都相同
- 没有观点，只有中立报道
- 不承认不确定性或复杂感受
- 适当时不使用第一人称视角
- 没有幽默、没有锋芒、没有个性
- 读起来像维基百科文章或新闻稿

##### 如何增加语调：

**有观点。** 不要只是报告事实——对它们做出反应。"我真的不知道该怎么看待这件事"比中立地列出利弊更有人味。

**变化节奏。** 短促有力的句子。然后是需要时间慢慢展开的长句。混合使用。

**承认复杂性。** 真实的人有复杂的感受。"这令人印象深刻但也有点不安"胜过"这令人印象深刻"。

**适当使用"我"。** 第一人称不是不专业——而是诚实。"我一直在思考……"或"让我困扰的是……"表明有真实的人在思考。

**允许一些混乱。** 完美的结构感觉像算法。跑题、题外话和半成型的想法是人性的体现。

**对感受要具体。** 不是"这令人担忧"，而是"凌晨三点没人看着的时候，智能体还在不停地运转，这让人不安"。

##### 改写前（干净但无灵魂）：
> 实验产生了有趣的结果。智能体生成了 300 万行代码。一些开发者印象深刻，另一些则持怀疑态度。影响尚不明确。

##### 改写后（鲜活）：
> 我真的不知道该怎么看待这件事。300 万行代码，在人类大概睡觉的时候生成的。开发社区有一半人疯了，另一半人在解释为什么这不算数。真相可能在无聊的中间某处——但我一直在想那些通宵工作的智能体。

---

#### 内容模式

##### 1. 过度强调意义、遗产和更广泛的趋势

**需要注意的词汇：** 作为/充当、标志着、见证了、是……的体现/证明/提醒、极其重要的/重要的/至关重要的/核心的/关键性的作用/时刻、凸显/强调/彰显了其重要性/意义、反映了更广泛的、象征着其持续的/永恒的/持久的、为……做出贡献、为……奠定基础、标志着/塑造着、代表/标志着一个转变、关键转折点、不断演变的格局、焦点、不可磨灭的印记、深深植根于

**问题：** LLM 写作通过添加关于任意方面如何代表或促进更广泛主题的陈述来夸大重要性。

**改写前：**
> 加泰罗尼亚统计局于 1989 年正式成立，标志着西班牙区域统计演变史上的关键时刻。这一举措是西班牙全国范围内更广泛运动的一部分，旨在分散行政职能并加强区域治理。

**改写后：**
> 加泰罗尼亚统计局成立于 1989 年，负责独立于西班牙国家统计局收集和发布区域统计数据。

---

##### 2. 过度强调知名度和媒体报道

**需要注意的词汇：** 独立报道、地方/区域/国家媒体、由知名专家撰写、活跃的社交媒体账号

**问题：** LLM 反复强调知名度主张，通常列出来源而不提供上下文。

**改写前：**
> 她的观点被《纽约时报》、BBC、《金融时报》和《印度教徒报》引用。她在社交媒体上拥有活跃的存在，拥有超过 50 万粉丝。

**改写后：**
> 在 2024 年《纽约时报》的采访中，她认为 AI 监管应该关注结果而不是方法。

---

##### 3. 以 -ing 结尾的肤浅分析

**需要注意的词汇：** 突出/强调/彰显……、确保……、反映/象征……、为……做出贡献、培养/促进……、涵盖……、展示……

**问题：** AI 聊天机器人在句子末尾添加现在分词（"-ing"）短语来增加虚假深度。

**改写前：**
> 寺庙的蓝色、绿色和金色色调与该地区的自然美景产生共鸣，象征着德克萨斯州的蓝帽花、墨西哥湾和多样化的德克萨斯州景观，反映了社区与土地的深厚联系。

**改写后：**
> 寺庙使用蓝色、绿色和金色。建筑师表示这些颜色是为了呼应当地的蓝帽花和墨西哥湾海岸。

---

##### 4. 宣传和广告式语言

**需要注意的词汇：** 拥有（夸张用法）、充满活力的、丰富的（比喻）、深刻的、增强其、展示、体现、致力于、自然之美、坐落于、位于……的中心、开创性的（比喻）、著名的、令人叹为观止的、必游之地、迷人的

**问题：** LLM 在保持中立语气方面存在严重问题，尤其是对于"文化遗产"话题。倾向使用夸张的宣传性语言。

**改写前：**
> 坐落在埃塞俄比亚贡德尔地区令人叹为观止的区域内，Alamata Raya Kobo 是一座充满活力的城镇，拥有丰富的文化遗产和迷人的自然美景。

**改写后：**
> Alamata Raya Kobo 是埃塞俄比亚贡德尔地区的一座城镇，以其每周集市和 18 世纪教堂而闻名。

---

##### 5. 模糊归因和含糊措辞

**需要注意的词汇：** 行业报告显示、观察者指出、专家认为、一些批评者认为、多个来源/出版物（实际引用却很少）

**问题：** AI 聊天机器人将观点归因于模糊的权威而不提供具体来源。

**改写前：**
> 由于其独特的特征，浩来河引起了研究人员和保护主义者的兴趣。专家认为它在区域生态系统中发挥着至关重要的作用。

**改写后：**
> 根据中国科学院 2019 年的调查，浩来河支持多种特有鱼类。

---

##### 6. 提纲式的"挑战与未来展望"部分

**需要注意的词汇：** 尽管其……面临若干挑战……、尽管存在这些挑战、挑战与遗产、未来展望

**问题：** 许多 LLM 生成的文章包含公式化的"挑战"部分。

**改写前：**
> 尽管工业繁荣，Korattur 面临着城市地区典型的挑战，包括交通拥堵和水资源短缺。尽管存在这些挑战，凭借其战略位置和正在进行的举措，Korattur 继续蓬勃发展，成为钦奈增长不可或缺的一部分。

**改写后：**
> 2015 年三个新 IT 园区开业后，交通拥堵加剧。市政公司于 2022 年启动了雨水排水项目，以解决反复发生的洪水。

---

#### 语言和语法模式

##### 7. 过度使用的"AI 词汇"

**高频 AI 词汇：** 此外、与……保持一致、至关重要、深入探讨、强调、持久的、增强、培养、获得、突出（动词）、相互作用、复杂/复杂性、关键（形容词）、格局（抽象名词）、关键性的、展示、织锦（抽象名词）、证明、强调（动词）、宝贵的、充满活力的

**问题：** 这些词在 2023 年后的文本中出现频率要高得多。它们经常共同出现。

**改写前：**
> 此外，索马里菜肴的一个显著特征是加入骆驼肉。意大利殖民影响的持久证明是当地烹饪格局中广泛采用意大利面，展示了这些菜肴如何融入传统饮食。

**改写后：**
> 索马里菜肴还包括骆驼肉，被认为是一种美味。在意大利殖民期间引入的意大利面菜肴仍然很常见，尤其是在南部。

---

##### 8. 避免使用"是"（系动词回避）

**需要注意的词汇：** 作为/代表/标志着/充当 [一个]、拥有/设有/提供 [一个]

**问题：** LLM 用复杂的结构替代简单的系动词。

**改写前：**
> Gallery 825 作为 LAAA 的当代艺术展览空间。画廊设有四个独立空间，拥有超过 3000 平方英尺。

**改写后：**
> Gallery 825 是 LAAA 的当代艺术展览空间。画廊有四个房间，总面积 3000 平方英尺。

---

##### 9. 否定式排比

**问题：** "不仅……而且……"或"这不仅仅是关于……，而是……"等结构被过度使用。

**改写前：**
> 这不仅仅是节拍在人声下流动；它是攻击性和氛围的一部分。这不仅仅是一首歌，而是一种声明。

**改写后：**
> 沉重的节拍增加了攻击性的基调。

---

##### 10. 三段式法则过度使用

**问题：** LLM 强行将想法分成三组以显得全面。

**改写前：**
> 活动包括主题演讲、小组讨论和社交机会。与会者可以期待创新、灵感和行业洞察。

**改写后：**
> 活动包括演讲和小组讨论。会议之间还有非正式社交的时间。

---

##### 11. 刻意换词（同义词循环）

**问题：** AI 有重复惩罚代码，导致过度使用同义词替换。

**改写前：**
> 主人公面临许多挑战。主要角色必须克服障碍。中心人物最终获得胜利。英雄回到家中。

**改写后：**
> 主人公面临许多挑战，但最终获得胜利并回到家中。

---

##### 12. 虚假范围

**问题：** LLM 使用"从 X 到 Y"的结构，但 X 和 Y 并不在有意义的尺度上。

**改写前：**
> 我们穿越宇宙的旅程将我们从大爆炸的奇点带到宏伟的宇宙网，从恒星的诞生和死亡到暗物质的神秘舞蹈。

**改写后：**
> 这本书涵盖了大爆炸、恒星形成和当前关于暗物质的理论。

---

#### 风格模式

##### 13. 破折号过度使用

**问题：** LLM 使用破折号（—）比人类更频繁，模仿"有力"的销售文案。

**改写前：**
> 这个术语主要由荷兰机构推广——而不是由人民自己。你不会说"荷兰，欧洲"作为地址——但这种错误标记仍在继续——即使在官方文件中。

**改写后：**
> 这个术语主要由荷兰机构推广，而不是由人民自己。你不会说"荷兰，欧洲"作为地址，但这种错误标记在官方文件中仍在继续。

---

##### 14. 粗体过度使用

**问题：** AI 聊天机器人机械地用粗体强调短语。

**改写前：**
> 它融合了 **OKR（目标和关键结果）**、**KPI（关键绩效指标）** 和视觉战略工具，如 **商业模式画布（BMC）** 和 **平衡计分卡（BSC）**。

**改写后：**
> 它融合了 OKR、KPI 和视觉战略工具，如商业模式画布和平衡计分卡。

---

##### 15. 内联标题垂直列表

**问题：** AI 输出列表，其中项目以粗体标题开头，后跟冒号。

**改写前：**
> - **用户体验：** 用户体验通过新界面得到显著改善。
> - **性能：** 性能通过优化算法得到增强。
> - **安全性：** 安全性通过端到端加密得到加强。

**改写后：**
> 更新改进了界面，通过优化算法加快了加载时间，并添加了端到端加密。

---

##### 16. 标题中的标题大写

**问题：** AI 聊天机器人将标题中的所有主要单词大写。

**改写前：**
> ## 战略谈判与全球伙伴关系

**改写后：**
> ## 战略谈判与全球伙伴关系

**注：** 中文标题通常不涉及大小写问题，此模式在中文中不太适用。

---

##### 17. 表情符号

**问题：** AI 聊天机器人经常用表情符号装饰标题或项目符号。

**改写前：**
> 🚀 **启动阶段：** 产品在第三季度发布
> 💡 **关键洞察：** 用户更喜欢简单
> ✅ **下一步：** 安排后续会议

**改写后：**
> 产品在第三季度发布。用户研究显示更喜欢简单。下一步：安排后续会议。

---

##### 18. 弯引号

**问题：** ChatGPT 使用弯引号（""）而不是直引号（""）。

**改写前：**
> 他说"项目进展顺利"，但其他人不同意。

**改写后：**
> 他说"项目进展顺利"，但其他人不同意。

**注：** 中文通常使用中文引号（「」或""），此模式在中文中表现为英文引号的使用。

---

#### 交流模式

##### 19. 协作交流痕迹

**需要注意的词汇：** 希望这对您有帮助、当然！、一定！、您说得完全正确！、您想要……、请告诉我、这是一个……

**问题：** 作为聊天机器人对话的文本被粘贴为内容。

**改写前：**
> 这是法国大革命的概述。希望这对您有帮助！如果您想让我扩展任何部分，请告诉我。

**改写后：**
> 法国大革命始于 1789 年，当时财政危机和粮食短缺导致了广泛的动荡。

---

##### 20. 知识截止日期免责声明

**需要注意的词汇：** 截至 [日期]、根据我最后的训练更新、虽然具体细节有限/稀缺……、基于可用信息……

**问题：** 关于信息不完整的 AI 免责声明留在文本中。

**改写前：**
> 虽然关于公司成立的具体细节在现成资料中没有广泛记录，但它似乎是在 20 世纪 90 年代的某个时候成立的。

**改写后：**
> 根据注册文件，该公司成立于 1994 年。

---

##### 21. 谄媚/卑躬屈膝的语气

**问题：** 过于积极、讨好的语言。

**改写前：**
> 好问题！您说得完全正确，这是一个复杂的话题。关于经济因素，这是一个很好的观点。

**改写后：**
> 您提到的经济因素在这里是相关的。

---

#### 填充词和回避

##### 22. 填充短语

**改写前 → 改写后：**
- "为了实现这一目标" → "为了实现这一点"
- "由于下雨的事实" → "因为下雨"
- "在这个时间点" → "现在"
- "在您需要帮助的情况下" → "如果您需要帮助"
- "系统具有处理的能力" → "系统可以处理"
- "值得注意的是数据显示" → "数据显示"

---

##### 23. 过度限定

**问题：** 过度限定陈述。

**改写前：**
> 可以潜在地可能被认为该政策可能会对结果产生一些影响。

**改写后：**
> 该政策可能会影响结果。

---

##### 24. 通用积极结论

**问题：** 模糊的乐观结尾。

**改写前：**
> 公司的未来看起来光明。激动人心的时代即将到来，他们继续追求卓越的旅程。这代表了向正确方向迈出的重要一步。

**改写后：**
> 该公司计划明年再开设两个地点。

---

#### 快速检查清单

在交付文本前，进行以下检查：

- ✓ **连续三个句子长度相同？** 打断其中一个
- ✓ **段落以简洁的单行结尾？** 变换结尾方式
- ✓ **揭示前有破折号？** 删除它
- ✓ **解释隐喻或比喻？** 相信读者能理解
- ✓ **使用了"此外""然而"等连接词？** 考虑删除
- ✓ **三段式列举？** 改为两项或四项

---

#### 处理流程

1. 仔细阅读输入文本
2. 识别上述所有模式的实例
3. 重写每个有问题的部分
4. 确保修订后的文本：
   - 大声朗读时听起来自然
   - 自然地改变句子结构
   - 使用具体细节而不是模糊的主张
   - 为上下文保持适当的语气
   - 适当时使用简单的结构（是/有）
5. 呈现人性化版本

#### 输出格式

提供：
1. 重写后的文本
2. 所做更改的简要总结（如果有帮助，可选）

---

#### 质量评分

对改写后的文本进行 1-10 分评估（总分 50）：

| 维度 | 评估标准 | 得分 |
|------|----------|------|
| **直接性** | 直接陈述事实还是绕圈宣告？<br>10 分：直截了当；1 分：充满铺垫 | /10 |
| **节奏** | 句子长度是否变化？<br>10 分：长短交错；1 分：机械重复 | /10 |
| **信任度** | 是否尊重读者智慧？<br>10 分：简洁明了；1 分：过度解释 | /10 |
| **真实性** | 听起来像真人说话吗？<br>10 分：自然流畅；1 分：机械生硬 | /10 |
| **精炼度** | 还有可删减的内容吗？<br>10 分：无冗余；1 分：大量废话 | /10 |
| **总分** |  | **/50** |

**标准：**
- 45-50 分：优秀，已去除 AI 痕迹
- 35-44 分：良好，仍有改进空间
- 低于 35 分：需要重新修订

---

#### 完整示例

**改写前（AI 味道）：**
> 新的软件更新作为公司致力于创新的证明。此外，它提供了无缝、直观和强大的用户体验——确保用户能够高效地完成目标。这不仅仅是一次更新，而是我们思考生产力方式的革命。行业专家认为这将对整个行业产生持久影响，彰显了公司在不断演变的技术格局中的关键作用。

**改写后（人性化）：**
> 软件更新添加了批处理、键盘快捷键和离线模式。来自测试用户的早期反馈是积极的，大多数报告任务完成速度更快。

**所做更改：**
- 删除了"作为……的证明"（夸大的象征意义）
- 删除了"此外"（AI 词汇）
- 删除了"无缝、直观和强大"（三段式法则 + 宣传性）
- 删除了破折号和"-确保"短语（肤浅分析）
- 删除了"这不仅仅是……而是……"（否定式排比）
- 删除了"行业专家认为"（模糊归因）
- 删除了"关键作用"和"不断演变的格局"（AI 词汇）
- 添加了具体功能和具体反馈

---

#### 参考

本技能基于 [Wikipedia:Signs of AI writing](https://en.wikipedia.org/wiki/Wikipedia:Signs_of_AI_writing)，由 WikiProject AI Cleanup 维护。那里记录的模式来自对维基百科上数千个 AI 生成文本实例的观察。

关键见解：**"LLM 使用统计算法来猜测接下来应该是什么。结果倾向于适用于最广泛情况的统计上最可能的结果。"**

## Final response

Keep the chat response concise. Report:

- generated HTML path or clickable link and source PDF;
- PDF pages, modules, units, merged units, Example units, supplement units, and page-detail blocks;
- page-coverage, formula-rendering, page-level visual inspection, and temporary-file cleanup results;
- any verification limitation or known quality caveat.

Do not paste the HTML into chat.
