#!/usr/bin/env node

import fs from "node:fs";
import path from "node:path";

function usage(message) {
  if (message) console.error(`error: ${message}`);
  console.error("usage: node audit-qna-html.mjs <lecture.html> [--id <question-block-id>]");
  process.exit(2);
}

function parseArgs(argv) {
  if (argv.length < 1) usage("missing HTML path");

  const result = { htmlPath: path.resolve(argv[0]), onlyId: null };
  for (let index = 1; index < argv.length; index += 1) {
    if (argv[index] !== "--id") usage(`unknown argument: ${argv[index]}`);
    if (!argv[index + 1]) usage("--id requires a value");
    result.onlyId = argv[index + 1];
    index += 1;
  }
  return result;
}

function decodeHtml(value) {
  return value
    .replaceAll("&amp;", "&")
    .replaceAll("&lt;", "<")
    .replaceAll("&gt;", ">")
    .replaceAll("&quot;", '"')
    .replaceAll("&#39;", "'")
    .replace(/&#(\d+);/g, (_, code) => String.fromCodePoint(Number(code)))
    .replace(/&#x([0-9a-f]+);/gi, (_, code) => String.fromCodePoint(Number.parseInt(code, 16)));
}

function stripTags(value) {
  return decodeHtml(value.replace(/<[^>]*>/g, " ")).replace(/\s+/g, " ").trim();
}

function lintTex(tex) {
  const errors = [];
  let braceDepth = 0;

  for (let index = 0; index < tex.length; index += 1) {
    if (tex[index] === "\\" && tex[index + 1] === "\\") {
      index += 1;
      continue;
    }
    if (tex[index] === "{") braceDepth += 1;
    if (tex[index] === "}") braceDepth -= 1;
    if (braceDepth < 0) {
      errors.push("extra closing brace");
      break;
    }
  }
  if (braceDepth > 0) errors.push(`unclosed braces: ${braceDepth}`);

  const stack = [];
  for (const match of tex.matchAll(/\\(begin|end)\{([^}]+)\}/g)) {
    const [, kind, environment] = match;
    if (kind === "begin") {
      stack.push(environment);
      continue;
    }
    const opened = stack.pop();
    if (opened !== environment) {
      errors.push(`environment mismatch: expected ${opened ?? "none"}, got ${environment}`);
    }
  }
  if (stack.length) errors.push(`unclosed environments: ${stack.join(", ")}`);
  return errors;
}

function findQuestionBlocks(html) {
  const blocks = [];
  const openingPattern = /<details\b[^>]*>/gi;

  for (const opening of html.matchAll(openingPattern)) {
    const start = opening.index;
    const end = html.indexOf("</details>", start);
    if (end < 0) continue;

    const source = html.slice(start, end + "</details>".length);
    const summaryMatch = source.match(/<summary\b[^>]*>([\s\S]*?)<\/summary>/i);
    const summary = summaryMatch ? stripTags(summaryMatch[1]) : "";
    const isQuestion = /\bdata-cslides-tutor\s*=\s*["']qna["']/i.test(opening[0])
      || /^(学生追问|Student question|Follow-up)\s*[:：]/i.test(summary);

    if (!isQuestion) continue;

    const idMatch = opening[0].match(/\bid\s*=\s*["']([^"']+)["']/i);
    blocks.push({
      id: idMatch?.[1] ?? null,
      opening: opening[0],
      source,
      summary,
    });
  }
  return blocks;
}

const { htmlPath, onlyId } = parseArgs(process.argv.slice(2));
if (!fs.existsSync(htmlPath)) usage(`file not found: ${htmlPath}`);
if (path.extname(htmlPath).toLowerCase() !== ".html") usage("target must be an .html file");

const html = fs.readFileSync(htmlPath, "utf8");
const errors = [];
const warnings = [];

const ids = [...html.matchAll(/\bid\s*=\s*["']([^"']+)["']/gi)].map((match) => match[1]);
const duplicateIds = [...new Set(ids.filter((id, index) => ids.indexOf(id) !== index))];
for (const id of duplicateIds) errors.push(`duplicate id: ${id}`);

const allBlocks = findQuestionBlocks(html);
if (onlyId && !allBlocks.some((block) => block.id === onlyId)) {
  errors.push(`question block not found: ${onlyId}`);
}
const blocks = onlyId ? allBlocks.filter((block) => block.id === onlyId) : allBlocks;

for (const [index, block] of blocks.entries()) {
  const label = block.id ?? `question-block-${index + 1}`;
  if (!block.id) errors.push(`${label}: missing id`);
  if (/\sopen(?:\s|=|>)/i.test(block.opening)) errors.push(`${label}: details must be closed by default`);
  if (!block.summary) errors.push(`${label}: missing summary`);
  if (!/<div\b[^>]*class\s*=\s*["'][^"']*\bsupplement-body\b/i.test(block.source)) {
    warnings.push(`${label}: no .supplement-body found; confirm this matches the page's component pattern`);
  }

  const withoutAttributesAndScripts = block.source
    .replace(/\bdata-tex\s*=\s*(["'])([\s\S]*?)\1/gi, "")
    .replace(/<script\b[\s\S]*?<\/script>/gi, "")
    .replace(/<style\b[\s\S]*?<\/style>/gi, "");
  if (/\\\(|\\\)|\\\[|\\\]/.test(withoutAttributesAndScripts)) {
    errors.push(`${label}: naked MathJax delimiter inside question block`);
  }

  for (const [formulaIndex, match] of [...block.source.matchAll(/\bdata-tex\s*=\s*(["'])([\s\S]*?)\1/gi)].entries()) {
    const raw = match[2];
    if (/&(?!amp;|lt;|gt;|quot;|#39;|#\d+;|#x[0-9a-f]+;)/i.test(raw)) {
      errors.push(`${label}: formula ${formulaIndex + 1} contains an unescaped ampersand`);
    }
    const tex = decodeHtml(raw);
    for (const texError of lintTex(tex)) {
      errors.push(`${label}: formula ${formulaIndex + 1}: ${texError}`);
    }
  }
}

const report = {
  file: htmlPath,
  checkedQuestionBlocks: blocks.length,
  discoveredQuestionBlocks: allBlocks.length,
  totalIds: ids.length,
  errors,
  warnings,
  ok: errors.length === 0,
};

console.log(JSON.stringify(report, null, 2));
process.exitCode = report.ok ? 0 : 1;
