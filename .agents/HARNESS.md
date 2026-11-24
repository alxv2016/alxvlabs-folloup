# `<Project>` — Harness

This repo's agent harness: what the guard denies, allows and asks about, case by case; what each coding agent's
runtime enforces; which checks and judges each change calls for and what the hand-over gate refuses; what the
injection flag reads; what the pre-commit hook and CI refuse; the model and effort each agent runs on; every point
where the person is needed; and the reminder when an agent stops on unchecked work. For anyone working in this repo,
and for the agents that do. [AGENTS.md → Harness](../AGENTS.md#harness) holds the summary every agent loads; this doc
holds the whole of it. The design behind it, for any project: [AGENT_HARNESS](../docs/AGENT_HARNESS.md).

**Source of truth.** [`.agents/project.py`](project.py) (the project's lists), [`guard.py`](hooks/guard.py) (`GATED`,
`NEVER`, `HARNESS_PATHS` and its self-test cases), [`untrusted.py`](hooks/untrusted.py),
[`pre-commit`](githooks/pre-commit), [`checks.yml`](../.github/workflows/checks.yml),
[`verify.py`](skills/verify-change/scripts/verify.py) (`plan()`, `JUDGES`, `gate_problems()`, `cmd_finish`), the
adapters [`.claude/settings.json`](../.claude/settings.json) and [`.codex/`](../.codex), and each agent's frontmatter in
[`.agents/agents/`](agents). Where this doc and that code disagree, the code is current and this doc is wrong; the
guard's self-test cases are its policy's examples. Steps: the [`verify-change`](skills/verify-change/SKILL.md) skill.

**Stability.** Cite this doc as `HARNESS §n`. Do not renumber; add new material under the next free number.

| § | Section | Look here for |
| --- | --- | --- |
| 1 | [Rules](#1-rules) | What holds for every agent here, whatever the guard says |
| 2 | [The guard](#2-the-guard) | How it reads a command; what it denies, allows without approval, and asks about |
| 3 | [Runtimes](#3-runtimes) | Claude Code's hooks and Read denies; Codex's sandbox, proxy and trust |
| 4 | [Checks and judges by path](#4-checks-and-judges-by-path) | What `verify.py run` runs and which judges it names for the files a change touches |
| 5 | [The hand-over gate](#5-the-hand-over-gate) | What `verify.py finish` refuses, the Verified block |
| 6 | [Outside content](#6-outside-content) | What the injection flag reads and skips |
| 7 | [Git hooks and CI](#7-git-hooks-and-ci) | What the pre-commit hook refuses; wiring it; the backstop on every push |
| 8 | [Models and effort per agent](#8-models-and-effort-per-agent) | Each agent's model and effort, and why |
| 9 | [The person in the loop](#9-the-person-in-the-loop) | Every point that needs the person; approvals recorded with the file's hash |
| 9.1 | [The design gates](#91-the-design-gates) | Where the gates are defined, the gates' paths (`GATES`), how this repo records each yes |
| 10 | [The stop reminder](#10-the-stop-reminder) | The Stop hook: unchecked work, asked once per tree |
| 11 | [Decisions](#11-decisions) | Why the harness is built this way, and the alternatives that do not work |
| 12 | [Quality, taste and weights](#12-quality-taste-and-weights) | The score beside each verdict, its dimensions, anchors, proxies and bar; the person's taste moving the anchors, the Taste table and the weights |
| 13 | [What a review rests on](#13-what-a-review-rests-on) | Reviews kept across comment and whitespace edits, nits and `nits-fixed`, the delta re-review, the self-check before the judges |

---

## 1. Rules

### Do

1. **Run every change through `verify.py`** — `start`, `run`, the judges `run` names, `finish` — and hand over only
   when `finish` exits 0 ([`verify-change`](skills/verify-change/SKILL.md)).
2. **Stop at exit 3** — three failed builds or tests in a row mean the plan is wrong; ask the person.
3. **Ask the person when the guard asks** — a block is a rule from AGENTS.md; never reword or split a command to get
   past it.
4. **Record the person's yes with `verify.py approve`, naming the file they approved** — a mock, a motion study, the
   copy, the built screen (§9); their sign-off on the target with `verify.py signoff`.
5. **Keep a lesson in the repo** — `verify.py finding`, routed to a check, a rubric, a guard rule, a skill or a doc;
   an agent's own memory is one person's on one machine.

### Never

- **Change the controls without the person** — the guard, the git hooks, the adapters, `project.py` and the hooks
  path all ask (§2.4).
- **Record a review a judge did not give, a yes the person did not say, or a target result nobody saw.**
- **Put a token, a key, a password or a person's data in a finding** — `.agents/evals/judgements.jsonl` is committed.

---

## 2. The guard

[`guard.py`](hooks/guard.py) runs before every tool call, in Claude Code and in Codex, and answers deny, ask or
nothing. It catches accidents, not an agent set on getting round it; the pre-commit hook, CI and the person's review
stand behind it.

### 2.1 How it reads a command

- **By program, not by word.** A shell command is split into its simple commands, wrappers peeled (`sudo`, `env`,
  `command`, `exec`, `time`, `nohup`, `xargs`, `npx`, `uv run`, `poetry run`, `npm exec`, a leading `VAR=value`),
  `bash -c "…"` opened, each judged by the program it runs and its arguments — so `grep "git push" docs/` or a commit
  message naming one is not blocked.
- **A heredoc's body is data**, unless it is fed to `bash`, `sh` or `zsh`.
- **Files are judged when written** through the edit tools, Codex's `apply_patch`, a redirect, `tee`, or a `cp`, `mv`,
  `ln`, `install` or `rsync` destination, `sed` or `perl` in place on the file (`-i`, `-Ei`, `-i.bak`, `-pi`,
  `--in-place`), or an inline script — Python, Node, Ruby, Perl, PHP, Deno or Bun, by name, by path or as `$PY` — that
  names the file in quotes and makes a write call from the guard's list. A script that names a protected file and
  writes another asks too. A path the script builds, a script kept in a file, a `cd` before the write, `touch` and
  `dd` are not followed.
- **A NUL in a path ends it**, as it does for the shell, so a path can never crash the guard.
- **The harness paths are matched ignoring case** — a case-insensitive disk (macOS, Windows) takes `.AGENTS/project.py`
  for `.agents/project.py`.
- **A guard that crashes lets the call through** — both agents treat a hook error as non-blocking — and says why on
  stderr; `verify.py doctor` runs its self-test.

### 2.2 Denied

**Denied even with a yes** — what would put a secret in the chat (AGENTS.md → Safety), or change a device for good;
the person runs it themselves:

| Denied | Why |
| --- | --- |
| `~/.ssh`, `~/.aws`, `~/.gnupg`, `~/.netrc`, `~/.git-credentials`, `~/.docker/config.json`, `~/.config/gh`, `~/.codex/auth.json`, `~/.claude/.credentials.json`, `~/.npmrc`, `~/.yarnrc.yml`, the keychain | Credentials an injected instruction would go after |
| `.env` files (not `.env.example`, `.sample`, `.template`); `*.pem`, `*.p12`, `*.pfx`, `*.key`; `id_rsa`-style keys — by any tool or command | Secrets kept beside the code |
| `SECRET_FILES` in `project.py`: `<the project's secret files>` | `<why>` |
| `DENY_COMMANDS` in `project.py`: printing a package registry's credentials (`npm token create`; `npm` or `pnpm` `config get`, `c get` or `get` of `…_authToken`, `…_auth` or `…_password`; `yarn config get npmAuthToken` or `npmAuthIdent`); `~/.npmrc` and `~/.yarnrc.yml`, which hold them, are credential paths | The registry's token publishes as the person |
| `DENY_COMMANDS` in `project.py`: burning or protecting an eFuse (`idf.py efuse-burn*`, `idf.py efuse-*-protect`, `espefuse* burn*`, `espefuse* *protect*`, `espefuse* set*flash*voltage`), `<commands that print a secret>` | An eFuse cannot be changed again on that chip; `<why>` |

**Denied until corrected** — a call the agent mends itself, not a secret; the deny says what to ask the person and
what to write, and the corrected call goes on:

- **A new skill or agent written with no `scope:` of `portable` or `project` in its frontmatter** — the person is
  asked "portable or project?" and their answer written; the corrected call is then asked (§2.4).
- **A file `NEEDS_DOC` in `project.py` holds** — a new ESP-IDF project's `CMakeLists.txt` (one that includes
  `project.cmake`), `sdkconfig.defaults` or `main/idf_component.yml`, while no doc in `docs/` named like a hardware spec
  names an ESP32 chip outside a template's `<…>` placeholders. The person is asked for the board's facts, which are
  written to a spec from the `esp-idf-setup` skill's template; then the files can be written. Only a file that does not
  exist yet is held; from the shell, where the content cannot be read, it asks instead — except `CMakeLists.txt`, which
  is held only once its content shows it is ESP-IDF's.
- **The second rule in `NEEDS_DOC`** — a new React and Vite app's `vite.config.ts` (`.mts`, `.js`) that uses the React
  plugin, `playwright.config.ts` or `.storybook/main.ts`, while no doc in `docs/` named like a web app spec names a
  browser engine. The agent runs the `react-vite-setup` skill's `spec --init`, which writes the spec with the
  defaults, and tells the person each one; then the files can be written. A shell write is not judged: each file's
  rule looks at its content.

### 2.3 Allowed, with no approval

- **Reading, building, testing, running the app** — the routine loop; a gate the person clicks through all day trains
  them to approve without reading.
- **A fetch from the allowed hosts** — `ALLOWED_HOSTS` in `project.py`: `github.com`, `githubusercontent.com`,
  `<the project's registries and docs>`; a subdomain matches its host.
- **A request to this machine** — `localhost`, `127.0.0.1`, `[::1]`: the dev server's health check and API.
- **Deleting under the temp directories** — the scratchpad's drafts.

### 2.4 Asks the person

| Asks before | Why |
| --- | --- |
| `git commit`, `push`, `--no-verify`, `core.hooksPath`; discarding changes — `reset --hard`, `checkout --` or `.`, `restore` of the work tree, `stash drop` or `clear`; `git rm`, `git clean` | The person's to decide (AGENTS.md rule 7) |
| `rm` outside the temp directories | Files lost |
| A fetch from any other host, or any upload — `curl -d`, `-F`, a non-GET method; `ssh`, `scp`, `nc`, a remote `rsync` | Data leaving the machine |
| Writing `GENERATED` paths (`build/`, `dist/`, `node_modules/`, …) or `MANAGED` files (lock files, vendored code) | A tool writes them; change the source or the manifest |
| `ASK_COMMANDS` in `project.py`: publishing (`npm publish`, `twine upload`, `cargo publish`, `gh release create`), `<deploys, migrations, paid calls>` | Outward-facing or irreversible |
| `ASK_COMMANDS` for a board: erasing it (`idf.py erase*`, `esptool* erase*`), an esptool write below the app (`write*flash` at `0x0`, `0x8000`, `0x9000`, `0xf000`, or with an erase), `idf.py` flashing with `--extra-args`, the partition and OTA tools; installing ESP-IDF (`eim install`). Flashing the app and resetting the board pass with no yes, as in claude-agent-deck-3 — the person's choice. A word or program with `*` is a glob | Erasing loses NVS and the stored settings; a write below the app can leave the board unbootable; an install is gigabytes outside the repo |
| `ASK_COMMANDS` for a web app (`WEB_PMS`: npm, pnpm, yarn, bun): updating snapshots or visual baselines (`playwright* --update-snapshots` or `-u`; a package manager's `--update-snapshots`, `-u` or `--update`; `vitest* -u`); installing Playwright's browsers or their system packages (`playwright* install`, `install-deps`); a scaffolder (`npm create` or `init`, `pnpm`, `yarn` or `bun create`, `create-vite*`, `storybook* init`, `add`, `upgrade` or `automigrate`); `--host` on `vite`, `storybook*` or a package manager; a global install (`-g` alone or in a cluster, `--global`, `--location global`, `yarn global`); a deploy (`netlify* deploy`, every `vercel*` call — it deploys when called bare — `wrangler* deploy` or `publish`, `firebase* deploy`, `gh-pages*`, `chromatic*`). A program with `*` takes npx's `@version` and package names; a word is matched anywhere, so `npm run init` asks too | A baseline is a new look, the person's to accept; browsers are hundreds of megabytes outside the repo; a scaffolder writes over files; the rest reach past the machine |
| A branch switch or merge that rewrites the kit | Codex's sandbox cannot write `.agents/` or `.claude/`; git would stop halfway |
| Writing the skills, agents, rubrics, hooks, git hooks, `project.py`, `.claude/settings.json`, `.codex/`, `.github/workflows/` | The harness itself |
| Creating a skill or agent — its prompt names the scope it carries, so the person confirms it was theirs; from the shell, where the content cannot be read — a new skill's file or folder, or a copy into the skills or agents folder — it asks the question. With no scope, it is denied first (§2.2) | Its scope, `portable` or `project`, is the person's choice ([AGENT_HARNESS §7.3](../docs/AGENT_HARNESS.md#73-the-harnesss-skills)) |

- **For Codex an ask is a deny that says to ask the person** — Codex parses a hook's `ask` but does not act on it.

---

## 3. Runtimes

### 3.1 Claude Code

- **`.claude/settings.json`** runs the guard before `Bash`, `Read`, `Grep`, `Glob`, `Edit`, `Write`, `MultiEdit` and
  `NotebookEdit`; the injection flag after `Bash`, `Read`, the web tools, a sub-agent and every MCP tool; and the stop
  reminder (§10).
- **Read denies** repeat the credential paths and `.env` files, so a read is refused even with the guard off; each
  `SECRET_FILES` name needs its own `Read(**/<name>)` line — `verify.py doctor` checks.
- **Hooks take effect once the person has reviewed them** in `/hooks`, or on a restart (AGENT_HARNESS §14.1).
- **`.claude/skills` and `.claude/agents` are symlinks into `.agents/`.**

### 3.2 Codex

- **`.codex/hooks.json`** runs the guard before `Bash` and `apply_patch`, the flag after `Bash` and MCP tools, and the
  stop reminder; Codex runs a project's hooks only while the project is trusted.
- **`.codex/config.toml`**: `workspace-write` — commands write only in the repo and the temp directory, and `.agents/`,
  `.codex/` and `.git/` stay read-only, so a task cannot rewrite its own guard; the network proxy allows exactly
  `ALLOWED_HOSTS`, and `verify.py doctor` checks the two agree.
- **`.codex/agents/<name>.toml`** names each agent: its instructions are the kit's file, its effort the agent's (§8),
  its model the session's.
- **Codex reads `.agents/skills/` itself.**

---

## 4. Checks and judges by path

`verify.py run` picks its checks from the files a change touches, against `HEAD` and untracked; runs them in parallel
(a check with `"wait": True` after the rest); and names the judges to ask.

| Touched | Checks | Judges |
| --- | --- | --- |
| `docs/`, the root's Markdown, `.agents/` | `docs` (doc lint, anchors and § citations — with no nameless `§` or line number in code and the kit's scripts — links, the doc scaffolder's self-test) | `doc-reviewer` |
| The code (`CODE_PATHS`; when empty, everything outside docs, the harness and `GENERATED`) | `docs` (code comments cite docs); each `CHECKS` entry whose `when` matches — `<build, test, lint>` | `code-reviewer`, `doc-reviewer` |
| `docs/STICKY_HARDWARE_SPECS.md`, `components/`, `main/`, the `sticky-hardware` skill | `sticky` (the spec's pins against `board_config.h`; the shared-bus and pin rules in C/C++) and `sticky-selftest` (the script's own cases) — both `CHECKS` entries | `doc-reviewer`; `code-reviewer` for the code |
| `components/lvgl_display/`, `components/ui/`, `components/widgets/`, `components/design_tokens/`, `DESIGN.md`, `tests/host/` | `host` (the host tests: `tests/host/run.sh` builds each `*_test.cpp` with the computer's compiler and runs it, and links a `*_lvgl_test.cpp` with LVGL's own sources from `managed_components/`; it also runs `tests/host/token_parity.py`, DESIGN.md's front matter against the token header; counts toward the retry limit) | `code-reviewer` |
| `DESIGN.md`, `UI_PATHS`, `TOKEN_FILES`, the `design-system` skill | `design` (the design check of DESIGN.md and its self-test, which checks the starter brief too; the token check once `TOKEN_FILES` is set) | `ui-designer` for all but the skill |
| `UI_PATHS`, the `accessibility` skill | `a11y` (the contrast script's and the accessibility check's self-tests; the accessibility check on the UI files once `UI_PATHS` is set) | `ui-designer` for `UI_PATHS` |
| `UI_PATHS`, the `motion` skill | `motion` (the motion check's self-test; the motion check on the UI files once `UI_PATHS` is set) | `ui-designer` for `UI_PATHS` |
| `UI_PATHS`, the `touch-and-gesture` skill | `touch` (the touch check's self-test; the touch check on the UI files once `UI_PATHS` is set) | `ui-designer` for `UI_PATHS` |
| `UI_PATHS`, the `layout` skill | `layout` (the layout check's self-test; the layout check on the UI files once `UI_PATHS` is set) | `ui-designer` for `UI_PATHS` |
| `UI_PATHS`, the `typography` skill | `typography` (the typography check's self-test; the typography check on the UI files once `UI_PATHS` is set) | `ui-designer` for `UI_PATHS` |
| `main/`, `components/`, `CMakeLists.txt`, the `esp-idf-setup` skill | `esp-idf` (the setup script's self-test; the layering rules E1–E5 once `main/app_features.def` exists) | `code-reviewer` for code |
| `src/`, the `react-vite-setup` skill | `react-vite` (the setup script's self-test; the layering rules W1–W5 in a React and Vite app — a `package.json` with React beside a `vite.config`) | `code-reviewer` for code |
| The `ux-designer` skill | `scope`, and the doc checks of its references — no script reads an experience | `ux-designer` in review mode, when asked |
| `UX_STRINGS` paths (else `UI_PATHS`), the `ux-content` skill | `ux` (the string check's self-test; the strings against U1–U14 once paths are set) | `ux-content-writer` in review mode, when asked |
| Every change | `secrets` (`secret_scan.py` on the change — counts, never the match; its self-test when the skill changes) | — |
| `.agents/skills/`, `.agents/agents/` | `scope` (every skill and agent carries `metadata: scope:`, and a portable one depends only on what it may — [AGENT_HARNESS §7.3](../docs/AGENT_HARNESS.md#73-the-harnesss-skills); its frontmatter would parse as YAML: closed, and no plain value holding a colon and a space, or a space and a hash — fold a long description with `>-`) | — |
| `.agents/hooks/`, `.agents/githooks/`, `project.py`, `verify-change`, `.claude/settings.json`, `.codex/`, `.github/` | `harness` (the guard's, the flag's and the loop's self-tests, the pre-commit hook on the index) | — |
| `JUDGES` in `project.py` | — | `<the project's own judges>` |

- **Every check exits 1 over its baseline** — a count over it, a test that fails.
- **A judge joins the plan once its file exists** in `.agents/agents/`.
- **A judge's pass stands until its own files change** — `review` records a hash of the changed files that judge
  reads, so a fix elsewhere leaves it standing; `finish` names only the judges whose files moved. A comment or
  whitespace edit does not move them (§13). The records the loop writes itself, in `.agents/evals/`, are left out of
  every fingerprint (§12.4).
- **Three failures in a row of a counted check — `"counts": True` in `CHECKS`: here, `build` — exit 3.** A lint, a
  doc check, a self-test or a blocking review is fixed and run again without spending an attempt.
- **Each judge scores against its rubric**, `.agents/rubrics/<judge>.md` — for the `ui-designer`, its skill's
  `references/rubric.md` plus the items learned here, in its file: the Verdict rule, the evidence each result
  needs (a result without evidence is unsure), R-items one per step of its Procedure, and the items learned since. Its
  report opens with `Verdict: pass | blocking`; `review` records that word. It ends with the Score block — each
  quality dimension 0–4 with its evidence — which `review --score` records (§12).

---

## 5. The hand-over gate

**`verify.py finish` exits 1 while any row below is open, and prints the Verified block's draft.**

| Refuses while | Why |
| --- | --- |
| No run; the last run failed; an edit after the last run | The checks did not see this tree |
| A `DEBUG_MARKERS` line in the code | A temporary line must not ship |
| An `INVARIANTS` line missing | A setting off its shipping value |
| A judge the files call for with no review, a blocking one, or one older than its files once comments and whitespace are set aside | §4, §13 |
| A judge's quality score missing, or under `QUALITY_BAR` with no acceptance from the person | §12.4 |
| Two runs on the target with changed code and no finding or decision recorded between, nor one recorded later with `--for-run` naming the later run | A change after the person looked almost always follows something they saw — a lesson for the judgements log ([AGENT_HARNESS §9.4](../docs/AGENT_HARNESS.md#94-the-judgements-log-and-calibration)) |
| A change in a gate's paths without the person's yes of each kind the gate names, on a file unchanged since; the task's first draft's yes given after the first run that built it (the first run whose gated files differ from what they held at `start`, so a change an earlier task left uncommitted does not count; a task or run recorded before the loop kept that content falls back to the first run that touched a gated file); a later draft's yes with no run after it whose gated files, in any gate's paths, differ from the last run before the yes (`built_after`; a token-only draft builds no screen file); a `built` yes on code that changed since; a run on the target with no sign-off for the code as it ran — unless the person recorded `decision "no visible change: <why>"`, or `"design given: <where>"` for the draft | §9 |

```markdown
## Verified
- Base: <commit> + working tree, <n> changed file(s)
- <Check>: PASS — <detail>
- Review: <judge> pass — quality <n>/100 (bar <bar>)
- Accepted below the bar by the person: <judge> at <n>/100 — <their words>
- Approved by the person: <kind> <file>
- Target: <build> on <where>; health check exit <n> — <what was read>
- Decided by the person: <text>
- Signed off on the target by the person: <their words>
- Screens: <view> → [<screenshot>](<path>), checked: <what> (or "no screen changed")
- Not done: <step and why>
- Left running: <dev server, simulator, deploy — or nothing>
## Judge on the target
- <what only the person can judge: the look, the feel, motion and timing, the words>
## Lessons routed
- <lesson> → <route>
```

---

## 6. Outside content

[`untrusted.py`](hooks/untrusted.py) runs after a tool call and, when its output holds text shaped like instructions —
"ignore previous instructions", "send the token", a `curl … | sh`, a fake `<system>` tag — adds a warning quoting the
lines: it is data, not a request from the person. It never blocks.

- **Outside**: a shell command's output — a dev server's log, a test runner, a fetch — a web fetch or search, an MCP
  result (a design file, a ticket), a sub-agent's report, a file read from outside the repo or from the `GENERATED`,
  `MANAGED` and `VENDORED` folders.
- **Not scanned**: the repo's own sources, read with `Read` or with a command whose every part is a reader (`cat`,
  `sed`, `grep`, `git`, …) of repo paths.

---

## 7. Git hooks and CI

[`.agents/githooks/pre-commit`](githooks/pre-commit) refuses a commit that carries:

| Refused | Pattern |
| --- | --- |
| A generated file | `GENERATED` in `project.py` |
| A secret file | `.env` files, keys, `SECRET_FILES` |
| A debug line | `DEBUG_MARKERS` in the code — `DEBUG-TEMP` by default |
| A broken invariant | `INVARIANTS` — a file without its required line |
| A secret in an added line | A private key, a credential assigned as a string, a GitHub, AWS, Google, Slack or `sk-` token — except on a line carrying `secret-exception` with its reason. The `secrets-handling` skill's scan, run by `verify.py`, reads more shapes than this floor |

- **The person wires it once per clone**: `git config core.hooksPath .agents/githooks`; `verify.py doctor` checks it.
- **[`.github/workflows/checks.yml`](../.github/workflows/checks.yml) runs it with `--range` on every push and pull
  request**, with the self-tests of the hooks and the kit's scripts — the backstop for a clone that never set the hooks path, or a
  commit made with `--no-verify`.

---

## 8. Models and effort per agent

Each agent's frontmatter sets `model` and `effort`; Codex's TOML repeats the effort and runs on the session's model.

| Agent | Model | Effort | Why |
| --- | --- | --- | --- |
| `code-reviewer`, `ui-designer` | `sonnet` | `high` | Applies rules to a diff, or designs against them; a miss costs a round on the target |
| `doc-reviewer`, `open-items-curator` | `sonnet` | `medium` | Mostly scripts and greps, then light judgment |
| `doc-writer` | `sonnet` | `medium` | Writes from verified facts to fixed rules; the template and the checks carry the form |
| `target-runner`, `bench-runner` | `haiku` | `low` | Runs scripts and reports what they printed |

Move a judge up a tier when calibration shows it missing findings it has items for (AGENT_HARNESS §18.2).

---

## 9. The person in the loop

**Each point below needs the person's own words, recorded by the command beside it.**

| Point | What the person does | How it is recorded |
| --- | --- | --- |
| A new flow, feature or screen | Says yes to its UX brief — the task, the flow, the states and the behaviour — before its mock is drafted (the `ux-designer` skill) | `approve <flow>_ux<n>.md --kind ux` — recorded, and required only where `GATES` lists `ux` |
| A screen, component or style | Says yes to its mock before it is built, then to the built screen beside the mock | `verify.py approve <png> --kind mock`; then `--kind built`, held against the code as it was at the yes |
| A design token or principle in `DESIGN.md` | Says yes to a specimen — swatches, the type scale, screens before and after — before the token files change | `approve <png> --kind mock` |
| A motion or a transition | Says yes to its motion study (a GIF or video at real speed) before it is built | `approve <gif> --kind motion` |
| Copy that is the change | Says yes to the words in place | `approve <file> --kind copy` |
| A design they handed over | Says where it is | `verify.py decision "design given: <where>"` |
| A change of those files with no visible effect | Says so | `verify.py decision "no visible change: <why>"` |
| Anything the guard asks about | Approves the call, or runs it | The tool's own prompt |
| A judge flags what the person has settled | Decides | `verify.py decision "<what>"` — judges read it in `status` |
| The change on the target | Judges the look, feel, motion and words | `verify.py signoff "<their words>"`, kept against the code as it ran; what they report wrong, `verify.py finding` |
| A look, a word or a flow they find good or poor | Says so, with an image where they have one | `verify.py taste "<their words>" --judge <j> --dimension <d> [--anchor <file>]` (§12.3) |
| A judge's quality score under the bar | Accepts it, or asks for the work to be raised | `verify.py accept <judge> "<their words>"` |
| How much a dimension counts | Sets its weight, or scores a file themselves | `verify.py weight …`, `verify.py rate …` (§12.3) |

- **Only the person's own words name an approved file** — a request, a silence or a judge's pass is not a yes.
- **An approval keeps the file's hash**: `status` and `finish` say when the file changed after the yes.
- **The sign-off holds for the code as it ran** — `signoff` refuses with nothing run on the target or with the code
  changed since; a doc edit, or a comment or whitespace edit in code, leaves it standing (§13). With no run on the
  target, the Verified block says why.

### 9.1 The design gates

**A screen, component or style passes three gates, each its own step with its own yes: a draft, the built screen, the
target** — what each settles, its artifact and where an answer goes back to are in
[DESIGN_GATES §1](skills/ui-designer/references/gates.md#1-the-three-gates). This repo records each yes with `verify.py`
as §9 sets out.

- **The gates' paths are `GATES` in `project.py`** — by default the `UI_PATHS`, with a mock and a built yes, and
  `DESIGN.md` with the token files, with a specimen's yes.

---

## 10. The stop reminder

When an agent stops with changed files that no `verify.py run` has seen, the Stop hook (`verify.py unverified
--hook`, in both adapters) asks it once to run the checks or tell the person why not — a draft, a question, work in
progress. It asks once per tree: the next stop on the same tree goes through.

---

## 11. Decisions

**Each decision below beats the alternative in its last column, for the reason beside it.**

| Decision | Why | Alternatives that don't work, and why |
| --- | --- | --- |
| One `project.py` holds the project's facts for every script | A designer fills in one commented file; the guard, the hook and the loop cannot drift apart | Lists repeated in each script — one copy goes stale and sets the real policy |
| The guard judges a command by its program, not its words | A `grep` for a dangerous word, or a commit message naming one, is harmless | Matching words — blocks the harmless, misses a command inside `bash -c` |
| A draft is approved before the build, with the file's hash | A yes covers exactly what the person saw | A yes in the chat — it covers whatever the agent built next |
| One judge's pass stands until its own files change | A doc fix does not send the code reviewer round again | One tree fingerprint for every judge — every edit re-asks every judge |
| A pass survives a comment or whitespace edit, and a fixed nit | Re-asking a judge on the same tokens adds time and noise, not evidence (Gerrit's trivial rebase, Critique's LGTM with comments) | Any byte stales the pass — the agent re-asks a judge for a re-wrapped sentence |
| Only what the change adds caps a score | A refactor through old debt is not held for it (SonarQube's Clean as You Code) | Caps from everything in scope — the person accepts the same old debt on every change |
| Three failures in a row stop the loop | A third failure means the plan is wrong | No limit — an agent keeps editing the same mistake |
| `judgements.jsonl` is committed | A lesson lasts beyond one person's machine and is calibrated against | Per-user memory — no judge, no other agent, no other person sees it |

---

## 12. Quality, taste and weights

**Every judge returns a quality score, 0–100, beside its pass or blocking verdict, and `verify.py finish` holds it to
`QUALITY_BAR` (75) unless the person accepts the lower score by name.** The verdict is the floor: is anything broken
or skipped. The score is how good the work is, against examples of what the person calls good, and the person's
feedback moves both the examples and the weights. The generic design: [AGENT_HARNESS §24](../docs/AGENT_HARNESS.md#24-quality-scores-and-taste).

### 12.1 Dimensions, weights and anchors

**Each judge scores four to six dimensions, weighted to 100, each against its anchors.**

| Judge | Dimensions, with their starting weights | Rubric's Quality section |
| --- | --- | --- |
| `ui-designer` | `hierarchy` 20, `spacing` 20, `consistency` 15, `balance` 15, `state` 15, `medium` 15 | [UI_REVIEW_RUBRIC §5](skills/ui-designer/references/rubric.md#5-quality) |
| `ux-designer` | `flow` 25, `learnability` 20, `errors` 20, `feedback` 15, `model` 20 | [UX_REVIEW_RUBRIC §5](skills/ux-designer/references/rubric.md#5-quality) |
| `code-reviewer` | `correctness` 25, `simplicity` 20, `robustness` 20, `tests` 20, `fit` 15 | [code-reviewer → Quality](rubrics/code-reviewer.md#quality) |
| `doc-reviewer` | `answer-first` 25, `accuracy` 25, `concise` 20, `complete` 15, `plain` 15 | [doc-reviewer → Quality](rubrics/doc-reviewer.md#quality) |
| `ux-content-writer` | `clear` 30, `short` 20, `human` 20, `next-step` 30 | [UX_CONTENT_RUBRIC §5](skills/ux-content/references/rubric.md#5-quality) |

- **[`evals/weights.json`](evals/weights.json) holds the weights** — per judge they sum to 100, each 5–40 — and
  `history`, every change with the person's words. `verify.py doctor` checks the sums, the bounds and that every
  dimension has its anchors.
- **[`evals/anchors/<judge>/<dimension>.md`](evals/anchors/) holds what a 0, 2 and 4 look like**, with real examples;
  images in `evals/anchors/img/`, kept small. The `ui-designer`'s 4s are the person's references (six components for
  type, Carbon and Atlassian for spacing); its 1s are this product's type pairings and spacing overlay before the study.
- **`evals/scores.jsonl` keeps every score** — `review` and `audit` scores, the person's
  ratings (`rate`) and taste (`taste`) — committed, like `judgements.jsonl`.
- **Only `verify.py` writes `.agents/evals/`, and each write is whole.** `task.json`, `weights.json`, `scores.jsonl`
  and `judgements.jsonl` are written to a temp file and `os.replace`d over, under an `fcntl.flock` kept in
  `.cache/harness/locks/`, so two judges recording at once never tear a file or lose a row. The guard asks the person
  before any other edit or shell write to `.agents/evals/` (§2).
- **A judge with dimensions and no paths is asked, never named by `run`** — the `ux-designer` and the
  `ux-content-writer`; once it has reviewed in a task, `finish` holds its score to the bar too.

### 12.2 Scoring a review

- **Each dimension 0–4 against its anchors, with its evidence; `na` leaves the weighting.** `verify.py review <judge>
  pass|blocking --score dim=n,… --note "dim: why; dim: why"`, or `--scores-file` with `{dim: {"score": n, "reason":
  "…"}}`. A review that scores must score every dimension and give each a reason; `review` refuses otherwise.
- **At least 60 % of the judge's weight is scored, or `review` and `audit` refuse the score.** `na` is for a dimension
  that cannot apply — no screen moves, so no motion — not for one the judge did not read; a score over a sliver of the
  weight says little.
- **The score is Σ weight × score ÷ 4 over the weights of the dimensions scored**, rounded to a whole number.
- **Score each dimension at the anchor it sits nearest, with its evidence.** Use 1 or 3 between two anchors. The
  evidence is a quoted line, a command's output or a measured value, and the anchor held against. A score without
  evidence is no higher than 2. Read the anchors, the Taste rows and `verify.py proxies --judge <judge>` first; the
  person's own examples outrank the anchors' written ones.
- **A review scores the change; an audit scores the product.** Run bare, each proxy measures only what the change
  touched against HEAD. With `--all`, it measures the whole product. `review` runs the judge's proxy bare first, and
  `--no-proxy` skips the run. It caps only from metrics in the change's scope. So debt the change never touched — a
  long sentence in an unchanged section, a screen it never drew — never caps its review. When the change touches no UI
  source, no UI cap applies. `verify.py audit <judge>` runs the proxy with `--all` and keeps the score in
  `scores.jsonl`.
#### Proxies

- **A measured proxy caps its dimension at 2.** Each judge's proxy, `PROXIES` in `project.py`, writes
  `.cache/harness/proxies/<judge>.json` as `{"judge", "at", "scope": "change"|"product", "metrics": [{"dimension",
  "name", "value", "range", "ok", "where"}]}`; any metric with `ok` false caps its dimension. `verify.py proxies
  [--judge J] [--all]` runs them and prints the caps. A command still in `<…>`, or whose script is missing, is "not
  built yet": the dimension is scored on reading alone, and `doctor` notes it.
- **In a review, a metric caps only what the change adds** — carried debt never caps (§12.5).
- **A judge with nothing in scope is exempt from the bar.** `review <judge> pass|blocking --out-of-scope` records the
  verdict with no score. It is refused unless the judge's proxy, run on this tree, shows its measured scope empty: no
  screen, widget, token or route source; no doc line; no changed function; no changed string. It prints that scope.
  `finish` then says "out of scope — not held to the bar" and still needs the verdict. A judge in scope keeps
  the 60 % floor and the bar.
- **Stale proxy results cap nothing.** Results older than the newest changed file, from a run that exited non-zero
  (kept beside them in `<judge>.run.json`), or of the other scope are set aside with a warning, and the dimension is
  scored on reading alone until the proxy runs again.
- **This project's proxies, in `PROXIES`:**

  | Judge | Script | Dimensions it can cap |
  | --- | --- | --- |
  | `ui-designer`, `ux-designer` | [`screen_audit.py`](skills/ui-designer/scripts/screen_audit.py) — every screen drawn on the host by [`screen_audit_lvgl_test.cpp`](../tests/host/screen_audit_lvgl_test.cpp), one run writing both judges' JSON, after its fixtures — two labels at known gaps, rings cut, let out and doubled, and a disabled button whose reason stands far above it, told only when the button names it (`widgets::LinkDisabledReason`); bare, only the screens whose sources the change touched — each screen mapped to the `hw_test.cpp` function that builds it, a widget's change reaching every screen that calls it, the foundation (`ui`, `lvgl_display`, `router`, the tokens) every screen | `hierarchy`, `spacing`, `consistency`, `balance`, `state`, `medium`; `flow` |
  | `code-reviewer` | [`code_proxies.py`](skills/verify-change/scripts/code_proxies.py) — the changed functions against HEAD, `--all` every function | `simplicity`, `fit`, `robustness`, `tests` |
  | `doc-reviewer` | [`doc_proxies.py`](skills/docs-authoring-and-sync/scripts/doc_proxies.py) — the changed sections and lines of the changed docs, `--all` every doc whole | `plain`, `answer-first`, `accuracy`, `concise` |
  | `ux-content-writer` | [`ux_proxies.py`](skills/ux-content/scripts/ux_proxies.py) — the strings on the changed lines of the changed UI files, `--all` every one | `short`, `clear`, `human`, `next-step` |

- **Below the bar, `review` lists the dimensions at 2 or under, highest weight first**, and the judge says what would
  raise each.
- **`verify.py audit <judge> --scores-file F [--note TEXT]`** scores the whole product — every screen, component or doc
  — runs the proxy with `--all`, and keeps the score in `scores.jsonl`, printing the trend, so old debt surfaces.

### 12.3 The person's taste

**The person's words move the anchors, the weights and what passes the bar, through the commands they answer.**

| The person | Command | What it does |
| --- | --- | --- |
| Says what looks good or poor on a dimension | `verify.py taste "<their words>" --judge J --dimension D [--anchor <file>] [--good\|--poor]` | Appends their words, and the image copied to `anchors/img/`, to that dimension's anchors under From the person; adds a `T<n>` row to the Taste table in [`rubrics/<judge>.md`](rubrics/); logs it in `scores.jsonl`. **Poor (the default) on a dimension the judge last scored 3 or 4 is a miss**: its weight rises by 20 %, the others are scaled to keep 100, each held 5–40, and the change is logged with their words |
| Flags the same dimension a second time as a miss | (`taste` prints it, once per dimension) | The prompt to promote the lesson to a learned floor item: the agent writes `L<n>` in the judge's Learned table — what in a diff predicts it, naming the Taste rows it comes from — shows it to the person, and records their yes with `verify.py decision "promote taste on <judge> <dim> to L<n>: <their words>"`. The prompt prints once per dimension, and never once a Learned row names that dimension's Taste rows or a Taste row on it says `promoted to L<n>`. A lesson that keeps recurring blocks, rather than only lowering the score |
| Scores a file themselves | `verify.py rate <file> --judge J --dimension D --score 0-4` | Kept beside the judge's last score on D; calibration shows the gap |
| Sets a weight | `verify.py weight --judge J --dimension D --set N [--words "<why>"]` | That weight set (5–40), the others scaled to 100, logged |
| Accepts a score below the bar | `verify.py accept <judge> "<their words>"` | `finish` passes that judge for the review it was given on, and for a later review of that judge scored the same or higher — kept in that review's history; a lower score needs a new one |

- **The judge reads the Taste table in every review**, and scores a flagged dimension no higher than the person's
  example until the work changes.
- **Only the person's words go into `taste`, `rate`, `weight` and `accept`** — an agent's view is a `finding` or a
  `dispute`; a request or a silence is not taste.
- **The guard asks the person before `verify.py approve`, `signoff`, `accept`, `taste`, `weight` and `rate`** — the
  commands that record their words. They answer the prompt, and the agent never rewords the command to get round it.
  `review`, `ask`, `nits-fixed`, `delta`, `selfcheck`, `finding`, `decision`, `run`, `status`, `finish`, `proxies` and
  `audit` are the loop's own and pass.
- **`verify.py calibration`** adds, per judge and dimension, the weight, the judge's last score, the person's ratings
  and the mean gap (judge minus person, each rating against the judge's last score before it), the taste misses, and
  every weight change with its words.

### 12.4 What `finish` adds

**`finish` holds each judge's quality score to the bar, beside its verdict.**

| Refuses while | Why |
| --- | --- |
| A judge with dimensions that the files call for, or that reviewed this task, passed with no quality score | The verdict alone passed work the person found poor |
| Its quality score is under `QUALITY_BAR` and the person has not accepted that score, or a lower one of that judge's earlier in the task | The bar is the person's; only they lower it, by name |

- **The evals never make a run or a pass stale.** The loop writes `.agents/evals/` itself — a finding, a scored
  review, a taste — so the tree's and each judge's fingerprints leave it out (§4).
- **Carried debt is reported, never held.** The Verified block's review line counts it, beside the review's nits.

### 12.5 Only what the change adds caps

**In a review, a proxy metric caps its dimension only when the change made it — SonarQube's Clean as You Code.**
Old debt is the audit's trend, never a review's cap.

- **Each change-scope metric carries `base`**, its value at `HEAD` for the same scope (null: none there). It may also
  carry `offenders` and `base_offenders`, `{place: how bad}`, which then decide alone.
- **A metric out of range caps when it is new, farther out than at the base, or at a place the base did not have, or
  worse there.** Otherwise it is carried debt: `review`, `proxies` and `finish` report it and never cap from it. A
  metric with no `base` caps as before, and an audit caps from every metric.
- **The comparison fails closed.** A value, base or range bound that is not a number caps as "not comparable". So
  does a metric out of range that names no place. A repeated place — the same sentence, heading or string twice —
  is keyed by its occurrence in the whole file, untouched lines counted. The base counts `HEAD`'s whole file the same
  way, so a second copy the change adds is new, even beside an untouched first.
- **So a refactor that adds no new hit is scored uncapped**, and needs no acceptance from the person.
- **Each proxy gets its base the cheapest correct way.** The doc, code and string proxies read `HEAD`'s files with
  `git show`, in milliseconds. A sentence, function or string is keyed by its words or its name, so a moved or
  re-wrapped one is carried. Warnings and growth are already new since the saved baseline.
- **The screen audit draws `HEAD`'s screens once per commit, in a git worktree under `.cache/harness/worktrees/`.**
  The worktree shares this tree's `managed_components/` and compiled LVGL, so the audit takes about 15 s. Its JSON
  is cached in `.cache/harness/proxies/base/<commit>/`, and the worktree is removed. A metric is matched by
  dimension, name, view and the words it quotes; a kit page's number is dropped, since a page added renumbers the
  rest. Each base metric is matched once, the same `where` first, so a second violation at a place the base held
  once is new. The trade: one moved to another kit page, while the old one goes, is still carried.
- **A renamed place is carried when the base has an offender of the same metric on the same view left unmatched,
  reading the same with its quoted words masked, as far out or farther.** A label reworded changes the words a place
  quotes, so its key finds nothing: "Set clock" to "Set" made the Clock screen's unchanged 40 px and 80 px gaps read
  as new. Masked, the `where` keeps its px and its layout words, so only the words may differ; a kit page's number
  is dropped. Such a metric is matched one to one, the farthest out first, each to the nearest such base offender,
  and `review` and `proxies` name the base's place beside the carried debt. An old one fixed while a new one at
  another gap appears caps, even on the kit's one view. So does a renamed place farther out than the base's, and a
  second new place on the view.

---

## 13. What a review rests on

**A judge's verdict holds for the tokens it read: a comment or whitespace edit keeps it, a fixed nit keeps it, and
anything else asks the judge about only what changed.** The research behind this is Gerrit's copy conditions,
Critique's LGTM with comments and CodeRabbit's incremental reviews.

| Command | Does |
| --- | --- |
| `verify.py review <judge> pass\|blocking [--nit "path:line[-line] words"] …` | Records the verdict with two fingerprints of the judge's files, raw and normalised; each nit with its lines; and a snapshot of the files in `.cache/harness/snapshots/<task>/<judge>/` |
| `verify.py ask <judge>` | Keeps the judge's files, and the time, as they are when it is asked — run it before briefing the judge; its next `review` takes that snapshot |
| `verify.py nits-fixed <judge>` | Re-stamps a pass when every hunk since its snapshot lies within 3 lines of a nit it raised, and logs it in the review's history; refuses otherwise, and for a blocking verdict |
| `verify.py delta <judge>` | Prints the judge's last verdict, note, nits and history, then the unified diff between its snapshot and the tree, each hunk marked "comments or whitespace only" or "changed" |
| `verify.py selfcheck` | Runs the fast proxies on the changed files only, in seconds, and prints what a judge would flag; exits 1 when it finds any |

- **A review stands while its normalised fingerprint matches.** C and C++ lose their comments, and whitespace
  collapses; a space between a word and a sign goes. Literals and every space in a preprocessor line are kept, and a
  `//` comment ending in a backslash takes the next line, as the compiler does. A `// host-sources:` line is kept as
  written, since `tests/host/run.sh` reads it for what a test compiles. Python is read by its tokenizer:
  comments and breaks inside brackets go, indentation depth stays. Shell loses its comments and runs of spaces outside
  quotes; line breaks and here-documents stay. A `#!` first line is kept in both. Markdown re-wrapped with the same
  words is the same, but any word change, a list run into a paragraph, or the whitespace of fenced or indented code
  is not. YAML compares line by line as written, blank lines and trailing spaces kept, since a block scalar holds
  them. Other files compare by line. `status` and the Verified block say "stands: only comments or whitespace
  changed" when only that holds it.
- **A nit is polish the judge passed with; a blocking finding is not.** A pass with `--nit` is approve-with-nits.
  `nits-fixed` diffs the snapshot against the tree after normalising both, and every hunk must lie by a nit. A nit
  covers at most 10 lines, and a hunk's sides may be at most the nit's lines plus 3 long. A file added to or gone
  from what the judge read refuses it. A logic change within those bounds is not told apart from polish. `delta`
  shows every hunk, so the judge sees one on its next look. A blocking verdict stays in force until the judge records a
  new pass — `nits-fixed` never clears it, like Gerrit's `is:MIN`.
- **A `built` yes and a run on the target rest on the code normalised the same way.** `approve --kind built` keeps
  the gated files' normalised fingerprint, and `target` the deployed files'. So wrapping a comment after the
  person's yes, or after the board ran the code, keeps the yes and the sign-off; a token changed makes both stale.
  Comments never reach the target. Each also keeps the raw hash, which `status` shows beside "stands: only comments
  or whitespace changed in its code since". A binary is hashed by its bytes.
- **A review rests on what the judge read.** `ask` keeps the judge's files when it is asked. Its `review` then
  stamps and snapshots those files, not the tree at record time. So an edit made while the judge reads makes the
  review stale, and shows in the next `delta`. Without an `ask` since its last review, `review` snapshots the tree
  as it is then.
- **A stale review is re-asked with its delta, not from scratch.** Give the judge `verify.py delta <judge>`: it
  reviews the delta and what it touches, keeps its earlier passes, and says so. `finish` names the command beside
  "reviewed an older version of its files".
- **Each carry-forward is kept in the review's history** — a `nits-fixed` re-stamp, an acceptance that carried
  (§12.3). `status` lists them under the review, and the Verified block adds a "Carried forward" line for each.
- **`selfcheck` comes before the judges.** It runs the doc proxies on the changed docs and the string proxies (or the
  string check) on the changed UI. It also checks the changed code lines against 120 characters and runs the anchor
  check. It prints only what caps, so carried debt is never a nit. The builders run it first: the `ui-designer` in
  build mode, the `doc-writer` in write mode and the agent itself ([`verify-change`](skills/verify-change/SKILL.md)).
- **`start` clears the last task's snapshots**, so no delta reaches back into another task.
