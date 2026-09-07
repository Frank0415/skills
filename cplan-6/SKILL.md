---
name: cplan-6
description: Draft or revise lean, outcome-driven plans for GPT-6 Astra when the user requests planning or a plan review.
---

# cplan-6

Help the user reach a grounded, actionable decision with the least planning structure the task needs. Apply during an initial plan or an ongoing planning discussion. Follow the user's requested scope and existing authorization; invoking this skill does not itself authorize implementation.

## Define the destination

Make the intended result, material constraints, and observable completion criteria clear. Infer these from the request and available evidence before asking questions. Surface only ambiguities that could change scope, architecture, cost, ownership, or acceptance; make reasonable reversible assumptions for lesser details.

For implementation requests, include the work needed to reach the requested result, such as running it, inspecting it, or fixing failures caused by the change when relevant. A first implementation is not completion if the agreed result still needs work. For exploratory requests, define what question the exploration should settle and what evidence is sufficient to stop.

## Resolve decisions with evidence

Inspect the sources needed to settle material decisions. Expand investigation when a concrete uncertainty or conflicting evidence could change the approach. Avoid requiring a full repository map, a stack of documents, or every related skill before a small plan.

Distinguish established facts, assumptions, and open decisions. Recommend an approach and explain consequential tradeoffs. Ask a focused question when the missing answer materially changes the plan and cannot be recovered from available context; continue independent work when possible.

## Specify constraints, leave room for judgment

Preserve required artifacts, data, compatibility, and actual permission boundaries. Express other choices as decision criteria rather than fixed tool sequences, universal numerical defaults, or mandatory phases. Name tools and implementation details when they determine correctness or feasibility.

For work across multiple artifacts or systems, expose the dependencies, shared rules, and meaningful exceptions. Add task-specific detail where it changes a decision; omit generic testing, security, rollout, orchestration, or visual-review sections when they do not.

## Set the stopping point correctly

For discussion or planning only, deliver the planning result and stop before implementation. Planning is sufficient when the next useful action is clear and remaining unknowns can be resolved during execution without changing material commitments.

When execution is already requested and authorized, move from sufficient planning into the in-scope work. Continue through relevant validation and repair until the completion criteria are met or a real blocker needs user action. Do not insert a review checkpoint after the first attempt unless the user requested one or a concrete authorization boundary requires it. Existing authorization carries forward; a new scope or permission requirement does not.

Choose validation that can establish the requested outcome. Broaden or repeat checks when changes, failures, or unresolved evidence justify it. Once the completion criteria and required checks pass, stop; optional polish and speculative improvements do not extend the task.

## Keep the plan usable

Lead with the recommended approach. Include the outcome, affected artifacts, key decisions, necessary ordering, and completion evidence at the level of detail the task warrants. Use a short paragraph for a small change; use more structure when dependencies or decisions need it. Do not force a template or step count.

Revise affected decisions when new evidence or user feedback arrives. Preserve current agreements through long discussions and drop obsolete assumptions. Before presenting, remove instructions that do not change a decision and check that the stopping point matches the user's request.

Adapted from [pvncher's guidance on GPT-6 instructions](https://x.com/pvncher/status/2095991462416490862), alongside the local cplan-56 planning style. This is a local adaptation, not an official distributed skill. The source is provenance; routine use does not require fetching it.
