# English techniques

Apply this file when the specification is in English. Sentence craft follows
SimpleEnglish / ASD-STE100 structural rules. Normative force uses BCP 14.
Project terms, identifiers, and quoted errors stay exact.

## Classify first

| Class | Purpose | Shape |
|---|---|---|
| **Normative** | A testable contract | One requirement, one BCP 14 keyword, named actor, condition, observable result |
| **Procedural** | Tell the reader what to do | Imperative; one instruction per sentence; 20 words |
| **Descriptive** | Explain what a thing is or does | Simple present/past/future; one topic; 25 words |

Classify each passage before rewriting. A note inside a procedure is descriptive.

## BCP 14 keywords

Put this convention near the start of the document and use the uppercase words only with this force:

- `MUST`: absolute requirement
- `MUST NOT`: absolute prohibition
- `SHOULD`: strong recommendation; valid exceptions can exist
- `SHOULD NOT`: discouraged behavior; valid exceptions can exist
- `MAY`: permission
- `OPTIONAL`: a feature or field that is not required

When a `SHOULD` has a material exception, write the exception or the decision criterion in the same requirement.

In descriptive and procedural sentences, write facts with `can`, `will`, and simple tenses. Keep BCP 14 keywords for normative sentences.

## Vocabulary

- One term for one concept through the whole document.
- Reuse the project term. Define a new term at first use or in a terminology table.
- Keep product names, API names, identifiers, commands, paths, literals, and units exact.
- Write a technical noun as a noun and a technical verb as a verb: send the event to the webhook; then deploy the service.
- Pick one of `config` / `configuration` / `settings` / `options` and keep it.
- Write multi-word nouns of three words or fewer. When a name needs more, write it in full once, then use a short form, or break it with `of` / `for` / `in`: the timeout value for the connection pool.
- Define an acronym at first use unless the project already treats it as the primary name.
- Use American English spelling.

### Write these instead of filler

If the word carries no fact, delete the phrase.

| Filler | Write |
|---|---|
| leverage, utilize | use |
| in order to | to |
| prior to | before |
| ensure | make sure that |
| it is worth noting that, it's important to, crucially | state the fact |
| simply, just, easily, seamlessly, effortlessly | (delete) |
| robust, powerful, comprehensive, performant | the measurable property |
| functionality | function, feature |
| enables you to, allows you to | you can |
| is designed to, aims to | what it does |
| facilitate | help |
| in the event that | if |
| due to the fact that | because |
| as needed, as necessary | the condition |
| and/or | `X, or Y, or both` or one of them |
| e.g. / i.e. / etc. | for example / that is / named items |
| gracefully handles | the actual behavior |
| out of the box | by default |
| under the hood | internally |

### Software verbs

Write the observable action:

| Instead of | Write |
|---|---|
| support JSON | accept `application/json` request bodies |
| handle errors | record the error and return HTTP `503 Service Unavailable` |
| manage sessions | create, read, renew, revoke, or delete the session |
| optimize performance | the latency, throughput, memory, or complexity target |
| secure the service | the authentication, authorization, encryption, audit, or isolation requirement |

Keep one verb per kind of action: clients `send` requests; services `return` responses; programs `read` and `write` storage; runtimes `throw` exceptions; APIs `return` errors; components `reject` invalid input; `retry` names a repeated failed action and includes count, interval, and stop condition.

## Verbs and tenses

- Use infinitive, imperative, simple present, simple past, simple future, and past participle as an adjective (`the cached response`).
- Use an `-ing` form as a technical noun (`logging`) or inside one.
- Use active voice. Name the actor when more than one actor could act.
- Describe an action with a verb: `validate the token`, `compress the file`.
- Keep articles and the word `that`. Write complete words: `cannot`, `do not`, `it is`.

## Sentences

- Procedural sentences: 20 words or fewer, warnings included.
- Descriptive sentences: 25 words or fewer.
- Count as one word: numbers with units, abbreviations, identifiers, quoted text, and hyphenated units. A backticked command such as `sqlpipe run --config sqlpipe.yaml` counts as one word.
- One instruction per procedural sentence, except actions that occur together.
- One topic per descriptive paragraph; about six sentences as a tripwire.
- One new fact per descriptive sentence.
- Use a vertical list for three or more parallel items or steps. Keep list items grammatically parallel.
- Connect related sentences with `Then` or `As a result` when the relationship is needed.
- Replace unclear `it`, `this`, `that`, and `they` with the noun.
- Replace `the above`, `as needed`, `normally`, `appropriate`, `soon` with a named object or measurable condition.
- When `or` can be inclusive or exclusive, write which.
- State whether ranges include their endpoints.
- Attach each modifier and exception to the requirement it changes.
- Split a second clause into its own sentence. Use a period or a list.

## Procedures, notes, warnings

- Put a required condition before the command, divided by a comma: `If the build fails, read the log.`
- Put the action before its result in procedures.
- Notes give information. They use the 25-word descriptive limit.
- Warnings: start with the command or condition, then the risk.

**Before:** Note that data loss may occur if the destructive flag is enabled against production.

**After:** CAUTION: Use the `--force` flag only against a non-production database. The flag deletes rows that do not match the source.

## Untouchables

Leave these exact: code blocks, inline code, identifiers, CLI commands, flags, file paths, quoted errors, log lines, product names, API names, config keys, numbers with units.

When the source omits a number, a cause, or an exact term, keep the general statement.

## Self-check

1. Count words in the three longest sentences. Split any procedural sentence over 20 words and any descriptive sentence over 25.
2. Search for `'ll`, `'re`, `'s`, `has been`, `have been`, `-ing` verbs after a comma, and semicolons. Rewrite each hit.
3. Search for `if` and `when`. Each stands at the start of its sentence, before the command.
4. Confirm one name per concept, including the config/settings choice.
5. Confirm every normative sentence has one BCP 14 keyword and one testable behavior.

## Example

**Before**

> The system should quickly handle invalid logins and provide an appropriate response.

**After**

> If a client sends an invalid password to `POST /sessions`, the service MUST return an HTTP `401 Unauthorized` status code. The response body MUST contain `code: "INVALID_CREDENTIALS"`. The service MUST NOT create a session.
