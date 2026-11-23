"""The one file a project fills in. Everything else in the harness is generic and reads its project facts from here:
the guard (.agents/hooks/guard.py), the injection flag (.agents/hooks/untrusted.py), the pre-commit hook
(.agents/githooks/pre-commit) and the loop (.agents/skills/verify-change/scripts/verify.py).

Fill it in once, when the harness is set up (docs/AGENT_HARNESS.md §17), and keep it true as the project grows: a new
check, judge, secret or generated folder lands here in the same change as the thing it covers. An empty list or dict
means "none yet" — the harness then runs only its generic checks (the doc lint and its own self-tests).

Paths are relative to the repo root. A pattern that ends in `/` matches everything under that folder; a pattern with
`*`, `?` or `[` is a glob (`*` also matches `/`, so `*.css` matches every CSS file in the repo); anything else matches
that exact file. Python 3.9+, standard library only — the guard imports this file, and a guard that cannot import it
lets every call through.

The guard asks the person before an agent edits this file: it is part of the harness's controls.
"""

# --- The project ------------------------------------------------------------------------------------------------

# The name used in doc titles (`# <PROJECT_NAME> — <Topic>`) and the prefix of the project's own docs
# (`docs/<DOC_PREFIX>_<TOPIC>.md`). Leave DOC_PREFIX empty to give no prefix to any doc.
PROJECT_NAME = "<Project>"
DOC_PREFIX = ""

# Docs that apply to any project and travel between repos unchanged: they name no project and link only to each
# other (DOCUMENTATION_GUIDELINES §3.1).
PROJECT_WIDE_DOCS = ("docs/AGENT_HARNESS.md", ".agents/skills/docs-authoring-and-sync/references/guidelines.md",
                     ".agents/skills/accessibility/references/criteria.md",
                     ".agents/skills/accessibility/references/focus.md",
                     ".agents/skills/touch-and-gesture/references/feedback.md",
                     ".agents/skills/secrets-handling/references/secrets.md",
                     ".agents/skills/ux-content/references/ux-content.md",
                     ".agents/skills/ux-content/references/rubric.md",
                     ".agents/skills/design-system/references/brief.md",
                     ".agents/skills/motion/references/motion.md",
                     ".agents/skills/touch-and-gesture/references/touch.md",
                     ".agents/skills/layout/references/layout.md",
                     ".agents/skills/typography/references/typography.md",
                     ".agents/skills/ux-designer/references/hci.md",
                     ".agents/skills/ux-designer/references/methods.md",
                     ".agents/skills/ux-designer/references/rubric.md",
                     ".agents/skills/ui-designer/references/gates.md",
                     ".agents/skills/ui-designer/references/rubric.md",
                     ".agents/skills/esp-idf-setup/references/esp-idf.md",
                     ".agents/skills/react-vite-setup/references/react-vite.md")

# --- Where things are -------------------------------------------------------------------------------------------

# The source code. Empty means "every file outside docs/, the harness, the root's Markdown and GENERATED".
#   Web:     ("src/", "app/", "public/", "package.json")
#   React and Vite (the react-vite-setup skill prints the list to paste): ("src/", "e2e/", ".storybook/", "public/",
#            "index.html", "package.json", "vite.config.ts", "vitest.config.ts", "playwright.config.ts")
#   Mobile:  ("ios/", "android/", "lib/", "pubspec.yaml")
#   Python:  ("src/", "tests/", "pyproject.toml")
#   ESP-IDF: ("main/", "components/", "CMakeLists.txt", "sdkconfig.defaults", "partitions.csv")
CODE_PATHS = ("main/", "components/", "tests/", "CMakeLists.txt", "sdkconfig.defaults", "partitions.csv")

# The files that change what a person sees: screens, components, styles, copy, icons. Empty: no UI gates and no
# ui-designer judge. Fill it in for any project with an interface.
#   Web:     ("src/components/", "src/pages/", "src/app/", "*.css", "*.scss", "src/styles/")
#   Mobile:  ("ios/*/Views/", "android/app/src/main/res/", "lib/widgets/", "lib/screens/")
#   ESP-IDF with LVGL: ("components/app_shell/screen_*", "components/ui_*/", "components/<feature>/*_screen.cpp")
UI_PATHS = ("components/app_shell/screen_*", "components/ui/", "components/widgets/", "components/lvgl_display/",
            "components/hw_test/")

# The design brief at the repo root: the product's principles and every design token by role, with its value per
# theme (DESIGN.md is the template's demo). The ui-designer and design-system skills read it first; the design check
# proves its contrast pairs meet their floors and its tokens exist in TOKEN_FILES. Changing it calls the ui-designer.
DESIGN_FILE = "DESIGN.md"

# The UI strings the ux-content skill's string check reads (UX_CONTENT §1–§6):
#   "paths":    the files that hold UI strings (patterns as above) — empty: UI_PATHS
#   "calls":    regular expressions for the calls whose string literals at the call's own level are UI text (each
#               argument, never one in a nested call); set, they REPLACE the check's defaults (web markup, SwiftUI
#               and UIKit, Android resources and Compose, LVGL 8 and 9)
#   "names":    proper nouns and product names that keep their capitals
#   "case":     "title" for a product that follows Apple's title case, which turns the title-case rule off
#   "baseline": the hits accepted today, each with its reason in a comment here
#   Web:     {"paths": ("src/", "public/locales/")}
#   ESP-IDF: {"paths": ("components/ui_*/", "main/"), "names": ("<Product>",)}
# The kit's widgets take their words through their own factories, which LVGL's calls do not name: MakeX / NewX (a
# button's, row's, card's, dialog's, field's words), detail::Line / Words, and the setters that take a reason.
UX_STRINGS = {"paths": (), "baseline": 0,
              "names": ("English", "United Kingdom", "Sam", "Priya", "Thursday", "Saturday"),
              "calls": (
    # LVGL's own text calls (the check's defaults for LVGL), kept since these replace the defaults
    r"\blv_label_set_text(?:_fmt|_static)?", r"\blv_checkbox_set_text(?:_static)?",
    r"\blv_dropdown_set_text(?:_static)?", r"\blv_dropdown_set_options(?:_static)?", r"\blv_dropdown_add_option",
    r"\blv_roller_set_options", r"\blv_textarea_set_placeholder_text",
    r"\blv_msgbox_add_(?:title|text|text_fmt|footer_button)", r"\blv_msgbox_create",
    r"\blv_list_add_(?:text|button|btn)", r"\blv_list_set_button_text", r"\blv_win_add_title",
    r"\blv_tabview_add_tab", r"\blv_tabview_set_tab_text", r"\blv_menu_page_create",
    r"\blv_menu_set_page_title(?:_static)?", r"\blv_span_set_text(?:_static|_fmt)?",
    # the kit's own factories and setters
    r"\b(?:ui::|widgets::|detail::)?(?:Make|New)[A-Z]\w*",
    r"\b(?:Line|Words)(?=\s*\()",
    r"\bSet\w*(?:Enabled|Reason)(?=\s*\()",
)}

# Where the design tokens live in code — the values DESIGN.md decides, in the platform's format. A screen takes its
# values from these files, never from a literal (the design-system skill). Empty: the token check and the design
# check's token-drift half do not run.
#   ("src/styles/tokens.css",)  ("design/tokens.json",)  ("lib/theme/tokens.dart",)
#   ESP-IDF with LVGL: ("components/ui_theme/include/ui_theme.h",) — constants named by role, colours as 0xRRGGBB
TOKEN_FILES = ("components/design_tokens/include/design_tokens.h",)

# Files a build or a tool writes. Never committed — the pre-commit hook refuses them — and the guard asks before an
# agent edits one. Keep these in .gitignore as well.
GENERATED = ("build/", "dist/", "out/", "coverage/", ".cache/", "node_modules/", ".venv/", "__pycache__/",
             "managed_components/", "sdkconfig", "sdkconfig.old",  # these three: ESP-IDF's
             "storybook-static/", "playwright-report/", "test-results/", "blob-report/", "*.tsbuildinfo")  # the web's

# Files a tool manages that ARE committed: lock files, vendored dependencies. The guard asks before an agent edits
# one by hand — change the manifest and let the tool regenerate it. Trim to your stack.
MANAGED = ("package-lock.json", "yarn.lock", "pnpm-lock.yaml", "poetry.lock", "uv.lock", "Pipfile.lock",
           "Cargo.lock", "Gemfile.lock", "Podfile.lock", "pubspec.lock", "go.sum", "vendor/",
           "dependencies.lock",  # ESP-IDF's component manager
           "*.spec.ts-snapshots/")  # Playwright's visual baselines: updated by `--update-snapshots`, which asks

# Third-party or generated folders whose text the injection flag treats as outside content, like a web page.
# GENERATED and MANAGED folders are included automatically.
VENDORED = ("third_party/",)

# --- Secrets ----------------------------------------------------------------------------------------------------

# The project's own secret files, by file name, wherever they sit. Denied to every agent from every tool, even with
# a yes, and refused by the pre-commit hook. `.env` files (not `.env.example`, `.sample` or `.template`), private
# keys (`*.pem`, `*.p12`, `*.pfx`, `*.key`, `id_rsa`-style) and the usual credential folders (`~/.ssh`, `~/.aws`, …) are
# denied already. Each name here also needs a `Read(**/<name>)` deny in .claude/settings.json — `verify.py doctor`
# checks it.
#   ("service-account.json", "api_key.txt")
SECRET_FILES = ()

# The project's own token shapes, beyond the common ones (private keys, GitHub, AWS, Google, Slack, `sk-` keys), for
# the secrets-handling skill's scan: (regular expression, what it is). The scan prints counts, never a match.
#   (r"\bmyco_live_[A-Za-z0-9]{32}\b", "a MyCo live key")
SECRET_PATTERNS = []

# Repo paths that hold secret patterns on purpose — a scanner's own rules, a test of them; the scan skips them whole.
# A single line is exempted in place instead, with `secret-exception` and its reason on that line.
SECRET_PATTERN_HOMES = ()

# --- The network ------------------------------------------------------------------------------------------------

# The hosts a build, a fetch and the docs need. A `curl` or `wget` to any other host asks the person; a subdomain
# matches its host. The one home of the list: .codex/config.toml repeats it for Codex's sandbox, and
# `verify.py doctor` checks the two agree.
#   npm: "npmjs.org", "npmjs.com"   Python: "pypi.org", "pythonhosted.org"   Your platform's docs host.
ALLOWED_HOSTS = ("github.com", "githubusercontent.com")

# --- What needs the person ---------------------------------------------------------------------------------------

# Commands that need the person's yes: (program, words that must all appear in its arguments, why). The program is
# matched by name, after wrappers (`sudo`, `env`, `npx`, `uv run`, `bash -c "…"`) are peeled. Words () matches
# every call of the program. A program or a word with `*`, `?` or `[` is a glob, matched whole (`esptool*`,
# `write[_-]flash`); a word that is a tuple is any of its alternatives — for tools that spell a command several ways.
# Name each real command rather than a loose glob: a word is matched against every argument, file names included.
# Publishing and writing a board are gated by default; add deploys, migrations, paid API calls and anything that
# loses data on the target.
FLASH_TARGETS = ("flash", "app-flash", "bootloader-flash", "partition-table-flash", "encrypted-flash",
                 "encrypted-app-flash", "dfu-flash", "erase-flash", "erase_flash", "erase-otadata", "erase_otadata")
ERASE_TARGETS = ("erase-flash", "erase_flash", "erase-otadata", "erase_otadata")
# An esptool write's arguments that reach below the app at 0x10000 (partitions.csv), or erase before it writes.
BELOW_APP = ("0", "00", "0x0", "0X0", "0x00", "0x0000", "0x8000", "0x9000", "0xf000", "0xF000", "--erase-all", "-e",
             "-[!-]*e*")
WEB_PMS = ("npm", "pnpm", "yarn", "bun")  # the package managers a web app's scripts run through
NPM_AUTH = ("*_authToken*", "*_auth", "*_password*")  # the registry credentials in an npm or pnpm config
ASK_COMMANDS = [
    ("npm", ("publish",), "publishes a package to the registry"),
    ("pnpm", ("publish",), "publishes a package to the registry"),
    ("yarn", ("publish",), "publishes a package to the registry"),
    ("twine", ("upload",), "publishes a package to the registry"),
    ("cargo", ("publish",), "publishes a crate to the registry"),
    ("gh", ("release", "create"), "publishes a release"),
    # ESP-IDF, as claude-agent-deck-3's guard has it (the person's choice): flashing and resetting the board need no
    # yes, so a change is validated on the glass without asking; what would lose data on it does — erasing, an
    # esptool write below the app (an image at 0x0, the partition table, NVS, PHY data), the partition and OTA tools.
    # Burning an eFuse is denied below. Installing ESP-IDF itself (gigabytes in the home folder) still asks.
    ("idf.py", (ERASE_TARGETS,), "erases the board's flash — NVS with it"),
    ("idf.py", (FLASH_TARGETS, ("--extra-args", "--extra-args=*")),
     "--extra-args goes to esptool unread, where an erase or an address below the app can hide"),
    ("ninja", (ERASE_TARGETS,), "erases the board's flash — NVS with it"),
    ("cmake", ("--build", ERASE_TARGETS), "erases the board's flash — NVS with it"),
    ("esptool*", (("erase[_-]flash", "erase[_-]region", "write[_-]flash[_-]status", "write[_-]mem"),),
     "erases the board's flash, or writes its status register or memory"),
    ("esptool*", (("write[_-]flash",), BELOW_APP),
     "writes below the app partition — the bootloader, the partition table, NVS or PHY data — or erases first"),
    ("parttool*", (("write[_-]partition", "erase[_-]partition"),), "writes a partition on the board"),
    ("otatool*", (("write[_-]ota[_-]partition", "erase[_-]ota[_-]partition", "erase[_-]otadata",
                   "switch[_-]ota[_-]partition"),), "writes the board's OTA partitions"),
    ("espota*", (), "uploads firmware to a board over the network"),
    ("openocd", ("*program_esp*",), "writes the board's flash through JTAG"),
    ("eim", (("install", "wizard"),), "installs ESP-IDF into the home folder"),
    ("idf_tools*", ("install",), "installs ESP-IDF's tools into the home folder"),
    # A web app: a visual baseline accepts a new look, so it is the person's yes; Playwright's browsers are hundreds
    # of megabytes in the home folder; a scaffolder writes over the repo (the react-vite-setup skill never
    # overwrites); `--host` serves the source to the local network; a global install changes every project; a
    # deploy publishes. A program is globbed where npx runs it by version or package name (`vitest@latest`,
    # `netlify-cli`). A word is matched anywhere in the arguments, so `npm run create` asks too.
    ("playwright*", (("--update-snapshots", "--update-snapshots=*", "-u"),),
     "rewrites the visual baselines — a new look, which is the person's to accept"),
    *[(pm, (("--update-snapshots", "--update-snapshots=*", "-u", "--update"),),
       "rewrites the snapshots or visual baselines — a new look, which is the person's to accept") for pm in WEB_PMS],
    ("vitest*", (("-u", "--update", "--update=*"),), "rewrites the test snapshots"),
    ("playwright*", (("install", "install-deps"),),
     "downloads browsers into the home folder, or installs system packages"),
    ("npm", (("create", "init"),), "a scaffolder writes files over the repo"),
    *[(pm, ("create",), "a scaffolder writes files over the repo") for pm in ("pnpm", "yarn", "bun")],
    ("create-vite*", (), "a scaffolder writes files over the repo"),
    ("storybook*", (("init", "add", "upgrade", "automigrate"),), "rewrites Storybook's config and package.json"),
    ("sb", ("init",), "rewrites Storybook's config and package.json"),
    *[(prog, (("--host", "--host=*"),), "serves the source to the local network")
      for prog in ("vite", "vite@*", "storybook*") + WEB_PMS],
    *[(pm, (("install", "i", "add"), ("--global", "--location=global", "-g*", "-[!-]*g*")),
       "installs a package for every project on the machine") for pm in ("npm", "pnpm", "bun")],
    ("npm", (("install", "i", "add"), "--location", "global"), "installs a package for every project on the machine"),
    ("yarn", ("global",), "installs a package for every project on the machine"),
    ("netlify*", ("deploy",), "publishes the site"),
    ("vercel*", (), "publishes the site — the Vercel CLI deploys when called bare, so every call asks"),
    ("wrangler*", (("deploy", "publish"),), "publishes the site"),
    ("firebase*", ("deploy",), "publishes the site"),
    ("gh-pages*", (), "publishes the site"),
    ("chromatic*", (), "publishes the Storybook"),
    # ("terraform", ("apply",), "changes real infrastructure"),
    # ("prisma", ("migrate", "deploy"), "migrates a shared database"),
    # ("kill", (), "stops a process — the person's dev server may be the one"),
]

# Commands denied even with a yes, because their output would put a secret into the chat, or they change a device
# for good: (program, words, why). The person runs them themselves.
#   ("kubectl", ("get", "secret"), "prints a cluster secret")
#   ("vercel", ("env", "pull"), "writes the project's secrets to a local file the agent would read")
DENY_COMMANDS = [
    # The package registry's credentials, printed: npm's and pnpm's `_authToken`, `_auth` and `_password`, yarn's token
    ("npm", ("token", "create"), "prints a new npm auth token"),
    *[(pm, (("config", "c"), "get", NPM_AUTH), "prints the registry's auth token") for pm in ("npm", "pnpm")],
    *[(pm, ("get", NPM_AUTH), "prints the registry's auth token") for pm in ("npm", "pnpm")],
    ("yarn", ("config", "get", ("*npmAuthToken*", "*npmAuthIdent*")), "prints the registry's auth token"),
    # ESP-IDF: an eFuse, once burnt or protected, cannot be changed again on that chip
    ("idf.py", (("efuse[_-]burn", "efuse[_-]burn[_-]key", "efuse[_-]write[_-]protect", "efuse[_-]read[_-]protect"),),
     "burns or protects an eFuse — permanent on the chip"),
    ("espefuse*", (("burn[_-]efuse", "burn[_-]key", "burn[_-]key[_-]digest", "burn[_-]bit", "burn[_-]block[_-]data",
                    "burn[_-]custom[_-]mac", "write[_-]protect[_-]efuse", "read[_-]protect[_-]efuse",
                    "set[_-]flash[_-]voltage"),), "burns or protects an eFuse — permanent on the chip"),
]

# Files a project makes only once a doc exists. Writing one that does not exist yet, while no doc in docs/ matches,
# is denied until corrected — the reason says what to ask the person and which doc to write (HARNESS §2.2). Each:
#   "files": path → a pattern the new file's content must show for the rule to apply (None: any content; a file
#            with a pattern is not judged from the shell, where its content cannot be read)
#   "doc":   (globs on the doc's file name, a pattern it must hold outside `<…>` placeholders) — case-insensitive
#   "ask":   what the agent asks the person, and where it writes the answers
#   "example": optional — a word the doc pattern accepts, for the guard's self-test when the pattern is no plain word
NEEDS_DOC = [
    {"files": {"CMakeLists.txt": r"project\.cmake", "sdkconfig.defaults": None, "main/idf_component.yml": None},
     "doc": (("*hardware*", "*board*spec*", "*hw*spec*"), r"\bESP32"),
     "ask": "an ESP-IDF project is generated from the board's hardware spec, and docs/ has none — ask the person for "
            "the chip or module, the flash, the PSRAM, the console, the display and its pins, and write their answers "
            "to a hardware spec in docs/ from the esp-idf-setup skill's template first"},
    {"files": {"vite.config.ts": r"plugin-react", "vite.config.mts": r"plugin-react", "vite.config.js": r"plugin-react",
               "playwright.config.ts": r"@playwright/test", ".storybook/main.ts": r"storybook"},
     "doc": (("*web*app*spec*", "*web*spec*", "*app*spec*"), r"\b(?:chromium|webkit|firefox|chrome|safari)\b"),
     "example": "chromium",
     "ask": "a React and Vite app's configs come from the web app spec, and docs/ has none — run the react-vite-setup "
            "skill's `react_vite.py spec --init`, which writes it with the defaults, tell the person each [DEFAULT] "
            "fact, and pass any they name"},
]

# Lines that must never be committed: a temporary debug marker, a focused test, a debug flag left on. A regular
# expression, matched in the code (CODE_PATHS) by the pre-commit hook and by `verify.py finish`. Mark every
# temporary line you add for debugging with `DEBUG-TEMP` in a comment, and these catch it.
#   JavaScript: r"DEBUG-TEMP|\bdebugger;|\.only\("   Python: r"DEBUG-TEMP|\bbreakpoint\(\)"
DEBUG_MARKERS = r"DEBUG-TEMP"

# Lines a file must always hold — a setting at its shipping value: (file, regular expression, why). The pre-commit
# hook and `verify.py finish` refuse a tree where the file lacks the line.
#   ("src/config.ts", r"^export const USE_MOCK_API = false;", "the shipped app talks to the real API")
INVARIANTS = []

# --- The loop: checks, judges and gates (docs/AGENT_HARNESS.md §10) -------------------------------------------

# The project's checks, which `verify.py run` picks by the files a change touches and runs in parallel. Each:
#   "run":  the command, as a list, run from the repo root; exit 0 is a pass. Its output goes to a log file, and only
#           the last lines of a failure are printed.
#   "when": the paths that call for it (patterns as above).
#   "wait": True to run it after the parallel ones — for a check that reads what another one writes.
#   "counts": True when its failure counts toward the retry limit — the build and the tests; a lint or a self-test
#           failure is fixed and run again without spending an attempt (.agents/HARNESS.md §4).
# A check whose program is not installed fails with "not found" — add the tool, or take the check out.
CHECKS = {
    # "build": {"run": ["npm", "run", "build"], "when": ("src/", "package.json"), "counts": True},
    # "test":  {"run": ["npm", "test", "--", "--run"], "when": ("src/", "tests/"), "counts": True},
    # "lint":  {"run": ["npm", "run", "lint"], "when": ("src/",)},
    # "types": {"run": ["npx", "tsc", "--noEmit"], "when": ("src/",)},
    # DESIGN.md follows Google's DESIGN.md spec and holds no kit tables; its colour roles and contrast pairs, on both
    # refreshes, are proved in its supporting doc.
    "design-tokens": {"run": ["python3", ".agents/skills/design-system/scripts/design_check.py", "--file",
                              "docs/STICKY_DESIGN_TOKENS.md"], "when": ("DESIGN.md", "docs/STICKY_DESIGN_TOKENS.md")},
    # The reTerminal Sticky's pins against board_config.h, and the shared-bus and pin rules (the sticky-hardware skill).
    "sticky": {"run": ["python3", ".agents/skills/sticky-hardware/scripts/sticky_check.py"],
               "when": ("docs/STICKY_HARDWARE_SPECS.md", "components/", "main/", ".agents/skills/sticky-hardware/")},
    # The firmware, from a shell with ESP-IDF 6.1.1 activated (idf.py is a shell function there, so its script is
    # run directly) — the build-run-test skill.
    "build": {"run": ["bash", "-c", 'python "$IDF_PATH/tools/idf.py" build'],
              "when": ("main/", "components/", "CMakeLists.txt", "sdkconfig.defaults", "partitions.csv"),
              "counts": True},
    "sticky-selftest": {"run": ["python3", ".agents/skills/sticky-hardware/scripts/sticky_check.py", "--test"],
                        "when": (".agents/skills/sticky-hardware/",)},
    # The host tests: the firmware's pure logic (glass_rules.h, swipe_rules.h) and the swipe against LVGL's own sources,
    # built and run with the computer's own compiler, no board and no ESP-IDF — the build-run-test skill.
    "host": {"run": ["bash", "tests/host/run.sh"],
             "when": ("components/lvgl_display/", "components/ui/", "components/widgets/", "tests/host/"), "counts": True},
}

# The project's own judges, beyond the kit's (doc-reviewer, code-reviewer, ui-designer): agent name → the paths
# that call for it. A judge joins the plan once its file exists in .agents/agents/.
#   {"security-reviewer": ("src/auth/", "src/api/")}
JUDGES = {}

# The person's yes each kind of change needs before it is handed over: (what, the paths that call for it, the kinds
# of yes). Kinds given before the build: "mock" (a still of a screen or component), "motion" (a GIF or video of a
# move), "copy" (the words). Given after: "built" (the built screen beside its mock) — it covers the code as it was
# at the yes. `verify.py approve <file> --kind <kind>` records each yes with the file's hash.
GATES = [
    ("a screen or component", UI_PATHS, ("mock", "built")),
    # A token value is seen on every screen at once: its change is drafted as a specimen (swatches, the type scale,
    # a screen or two before and after) and approved before the token files change.
    ("a design token or principle", (DESIGN_FILE,) + TOKEN_FILES, ("mock",)),
]

# The quality bar: each judge's quality score, 0–100 from its dimensions in .agents/evals/weights.json, that
# `verify.py finish` asks for beside its pass — below it, the hand-over waits until the score is raised or the person
# accepts it by name (`verify.py accept <judge> "<their words>"`; .agents/HARNESS.md §12).
QUALITY_BAR = 75

# Each judge's measured proxies: judge → the command, as a list run from the repo root, that writes
# .cache/harness/proxies/<judge>.json as
#   {"judge": J, "at": iso, "scope": "change" | "product",
#    "metrics": [{"dimension": D, "name": str, "value": num, "range": [lo, hi], "ok": bool, "where": str}]}
# Run bare, the command measures only what the change touched — `verify.py review` runs it so; with `--all` it
# measures the whole product — `verify.py audit` adds it. A metric with ok false caps its dimension at 2, from results
# of the scope asked for, newer than every changed file, from a run that exited 0. A command still in `<…>`, or whose
# script does not exist, is "not built yet": the judge scores on its reading alone, and `verify.py doctor` notes it.
PROXIES = {
    # The screen audit draws every screen on the host (tests/host/screen_audit_lvgl_test.cpp) and writes both the
    # ui-designer's and the ux-designer's JSON in one run; a later run reuses the compiled LVGL, about 10 s.
    "ui-designer": ("python3", ".agents/skills/ui-designer/scripts/screen_audit.py"),
    "ux-designer": ("python3", ".agents/skills/ui-designer/scripts/screen_audit.py"),
    "code-reviewer": ("python3", ".agents/skills/verify-change/scripts/code_proxies.py"),
    "doc-reviewer": ("python3", ".agents/skills/docs-authoring-and-sync/scripts/doc_proxies.py"),
    "ux-content-writer": ("python3", ".agents/skills/ux-content/scripts/ux_proxies.py"),
}

# Where the change finally runs, in a few words — the Verified block names it.
#   "the app in a browser at localhost:3000"  "the iOS simulator"  "a phone over USB"  "the staging site"
#   "the ESP32-S3 board over USB, its serial console on the same port"
TARGET = "a reTerminal Sticky over USB (V1.0 SSD1677 and V2.0 SSD2677), its console on UART0 through the CH343P"

# Extra guard self-test cases for the project's own gates: (command, "deny" | "ask" | None). Give each gate a case,
# and a near-miss that must pass.
#   ("vercel --prod", "ask"), ("vercel dev", None)
GUARD_CASES = [
    # ESP-IDF: flashing the app passes; erasing, a write below the app and --extra-args ask; an eFuse is denied
    ("idf.py -p /dev/cu.usbmodem1101 flash monitor", None), ("idf.py app-flash", None),
    ("idf.py -p /dev/cu.wchusbserial1 flash", None), ("idf.py flash --extra-args=-e", "ask"),
    ("idf.py erase-flash", "ask"), ("idf.py erase_otadata", "ask"), ("idf.py build", None), ("idf.py monitor", None),
    ("idf.py size", None),
    ("python -m esptool --port /dev/x write-flash 0x0 a.bin", "ask"), ("esptool.py write_flash 0 merged.bin", "ask"),
    ("esptool.py write_flash 0x8000 partition-table.bin", "ask"), ("esptool.py write_flash 0x9000 nvs.bin", "ask"),
    ("esptool.py write_flash --erase-all 0x10000 app.bin", "ask"), ("esptool write-flash -ze 0x10000 app.bin", "ask"),
    ("esptool.py write_flash -z 0x10000 app.bin", None),
    ("python -m esptool --chip esp32s3 -p /dev/x -b 460800 write-flash 0x10000 folloup.bin", None),
    ("esptool.py erase_flash", "ask"), ("esptool.py erase_region 0x9000 0x6000", "ask"), ("esptool.py chip_id", None),
    ("idf.py efuse-burn --do-not-confirm DIS_USB_JTAG 1", "deny"), ("idf.py efuse-write-protect X", "deny"),
    ("espefuse.py burn_key BLOCK_KEY0 key.bin", "deny"), ("espefuse --port /dev/x burn-efuse X 1", "deny"),
    ("espefuse.py write_protect_efuse X", "deny"), ("espefuse.py set_flash_voltage 3.3V", "deny"),
    ("espefuse.py summary", None), ("idf.py efuse-summary", None),
    ("eim install -i v6.1", "ask"), ("eim wizard", "ask"), ("eim list", None),
    # the same writes, spelled through the build tool, a runner or a shell
    ("ninja -C build flash", None), ("cmake --build build --target app-flash", None),
    ("ninja -C build erase-flash", "ask"), ("cmake --build build --target erase-flash", "ask"),
    ("uvx --from esptool esptool write-flash 0x0 a.bin", "ask"), ("pipx run esptool erase-flash", "ask"),
    ('bash -lc "idf.py flash"', None), ('bash -c -l "idf.py erase-flash"', "ask"),
    ("timeout 60 idf.py -p /dev/x flash", None),
    ("parttool.py write_partition --partition-name nvs --input n.bin", "ask"), ("otatool.py erase_otadata", "ask"),
    ("python -m espefuse burn_efuse X 1", "deny"),
    # near misses: a file or an option value that only looks like a write
    ("esptool.py read_flash 0 0x1000 write.bin", None), ("idf.py -B build-flash build", None),
    ("espefuse.py --port protect.log summary", None), ("espefuse.py execute_scripts burn.py", None),
    ("espefuse.py summary > burn.txt", None), ("ninja -C build", None), ("cmake --build build", None),
    # A web app: a new look, a download, a scaffold, the network, a global install or a deploy asks; npm's token is
    # denied; the everyday loop passes
    ("npx playwright test --update-snapshots", "ask"), ("npx playwright test -u", "ask"),
    ("npm run test:e2e -- --update-snapshots", "ask"), ("npx vitest run -u", "ask"),
    ("npx playwright install chromium webkit", "ask"), ("npx -y playwright@1.65.1 install --with-deps", "ask"),
    ("npm create vite@latest my-app", "ask"), ("npx create-vite my-app", "ask"), ("npx storybook@latest init", "ask"),
    ("npx netlify deploy --prod", "ask"), ("vercel --prod", "ask"), ("npx wrangler pages deploy dist", "ask"),
    ("npm run dev -- --host", "ask"), ("npx vite preview --host=0.0.0.0", "ask"), ("npm install -g pnpm", "ask"),
    ("npx vitest@latest run -u", "ask"), ("npx netlify-cli deploy", "ask"), ("npx firebase-tools deploy", "ask"),
    ("npx vercel@latest deploy", "ask"), ("npx wrangler@latest deploy", "ask"), ("wrangler publish", "ask"),
    ("npx wrangler pages publish dist", "ask"), ("npx wrangler dev", None), ("vercel", "ask"),
    ("vercel --prebuilt", "ask"), ("npx playwright install-deps", "ask"), ("sudo npx playwright install-deps", "ask"),
    ("npm test -- -u", "ask"), ("npm run test:unit -- --update", "ask"), ("pnpm test:e2e --update-snapshots", "ask"),
    ("pnpm run test:e2e -- -u", "ask"), ("yarn test:e2e --update-snapshots", "ask"), ("pnpm dev --host", "ask"),
    ("yarn dev --host", "ask"), ("bun run dev --host", "ask"), ("npx storybook dev --host 0.0.0.0", "ask"),
    ("pnpm install -g x", "ask"), ("yarn global add x", "ask"), ("bun add -g x", "ask"), ("npm i -gD x", "ask"),
    ("npm install --location global x", "ask"), ("bun create vite my-app", "ask"),
    ("npm token create", "deny"), ("npm config get //registry.npmjs.org/:_authToken", "deny"),
    ("npm get //registry.npmjs.org/:_authToken", "deny"), ("npm c get //registry.npmjs.org/:_authToken", "deny"),
    ("npm config get _auth", "deny"), ("pnpm config get //registry.npmjs.org/:_authToken", "deny"),
    ("yarn config get npmAuthToken", "deny"), ("cat ~/.npmrc", "deny"), ("cat ~/.yarnrc.yml", "deny"),
    ("npm i -D vitest", None), ("pnpm add -D vitest", None), ("npm run lint -- --ignore-pattern x", None),
    ("npm ci", None), ("npm install", None), ("npm install -D vitest", None), ("npm run dev", None),
    ("npx playwright test", None), ("npx playwright test e2e/update-snapshots.spec.ts", None),
    ("npx playwright show-report", None), ("npx vitest run --project unit", None), ("npx storybook dev -p 6006", None),
    ("npm run build-storybook", None), ("npm ls -g", None), ("npm config get registry", None), ("vitest run", None),
    ("npx vite build", None), ("pnpm dev", None), ("npx playwright test --project chromium-390x844", None),
    ("cat .npmrc", None),
]


# --- Helpers — the harness's scripts call these; leave them as they are -----------------------------------------

HARNESS_DIRS = (".agents/", ".claude/", ".codex/", ".github/")


def matches(path, patterns):
    """Whether a repo-relative path matches any of the patterns (a folder with `/`, a glob, or an exact file)."""
    import fnmatch
    for p in patterns:
        if p.endswith("/"):
            # a folder: everything under it; with a glob in it (`components/ui_*/`), everything under each match
            if path.startswith(p) or (any(c in p for c in "*?[") and fnmatch.fnmatch(path, p + "*")):
                return True
        elif any(c in p for c in "*?["):
            if fnmatch.fnmatch(path, p):
                return True
        elif path == p:
            return True
    return False


def is_doc(path):
    """docs/, and the Markdown at the repo root."""
    return path.startswith("docs/") or ("/" not in path and path.endswith(".md"))


def is_code(path):
    """A source file: in CODE_PATHS, or — with CODE_PATHS empty — anything that is not a doc, the harness, or
    generated."""
    if CODE_PATHS:
        return matches(path, CODE_PATHS)
    return not (is_doc(path) or path.startswith(HARNESS_DIRS) or matches(path, GENERATED)
                or path in (".gitignore", ".mcp.json", "LICENSE"))


def code_pathspec():
    """A git pathspec for the code, for `git grep` and `git diff`."""
    if CODE_PATHS:
        return [p if not any(c in p for c in "*?[") else f":(glob){p}" for p in CODE_PATHS]
    return [".", ":(exclude)docs", ":(exclude)*.md", ":(exclude).agents", ":(exclude).claude", ":(exclude).codex",
            ":(exclude).github"] + [f":(exclude){g.rstrip('/')}" for g in GENERATED]
