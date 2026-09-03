# Contributing

Guidelines for branch naming, commit messages, and commit hygiene for this
repository.

## Branches

Branches use one of the following prefixes, reflecting the kind of work:

- `feature/` — new functionality
- `improve/` — optimization or refinement of existing code
- `refactor/` — internal restructuring with no behavior change
- `fix/` — bug fixes

The prefix is followed by a short, kebab-case description, e.g.
`feature/transaction-commit`, `improve/sessions-read`, `refactor/symmetric-node`,
`fix/port-invalid-range`.

A branch should address **one** focused piece of work. If work naturally splits
into several independent concerns, split them into separate branches rather than
stacking them.

## Commit messages

### Format

```
TYPE[MODULE]: MESSAGE
```

- `TYPE` — `feat`, `improve`, `fix`, `refactor`, `chore`.
- `MODULE` — a short scope hint, e.g. `core`, `ui`, `tests`, `workflows`,
  `config`. Omit the brackets only when no scope applies.
- `MESSAGE` — a concise imperative summary of the change.

Examples:

```
feat[core]: persistent sessions with tx_id multiplexing
fix[ui]: avoid temp string alloc on command lookup
improve[tests]: cover garbage frames and empty payload on read
chore: add new lines at the end of all files
```

### Body

The body is **almost always omitted**. It is the rare exception, reserved for
commits that are complex or genuinely need extra explanation. When a body is
used:

- Prefer a **single line with no random line breaks**.
- Or, when several points are needed, write them as a **list**.
- The body is **not a story** — state dry facts and give the briefest
  possible explanation. **Do not** elaborate, describe the *what*, or restate
  things that are obvious from `git diff` (for example, the list of files
  touched).

Example of an acceptable body (list + a single closing line), modeled on
commit `1b035d1`:

```
refactor: symmetric Node replaces separate server and client

 - Node: acceptor, thread pool, outgoing dials
 - NodeSession - symmetric WS session: accept/dial branches converge into a shared read/write loop

main.cpp and CMakeLists.txt collapse two binaries (vsna_server + vsna_client) into one (vsna).
```

If the commit is a **fix**, end the body with a `Fixes` line pointing at the
commit that introduced the bug:

```
Fixes a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2
```

`FULL_HASH_OF_COMMIT_CONTAINS_BUG` should be the full hash of the offending
commit, not an abbreviated one.

## Commit hygiene

- **1 commit = 1 micro-feature.** A commit must not mix unrelated changes. A
  message like `feat: add socket + refactor main` is a red flag — split it into
  two commits.
- **Every commit must be working, tested, and formatted.** Each individual
  commit should build, pass the tests, and be `clang-format` clean.
- **No commits that fix earlier commits within the same branch.** A branch must
  not contain "fix my earlier commit" follow-ups. Corrections are done by
  rewriting history, not by piling on new commits.
  - If the mistake is in the **last** commit — use `git commit --amend`.
  - If it is in an **earlier** commit — use a `fixup` commit and/or
    `git rebase -i`.
- Use **interactive rebase** also when you need to **split** a commit.
- **Do not be afraid to rewrite history** — this is expected before a branch is
  pushed or merged.
- **Never** use `git add .` or `git add -A`. Stage the intended changes
  explicitly: `git add -u` for modifications to tracked files, plus explicit
  `git add <path>` for new files.
