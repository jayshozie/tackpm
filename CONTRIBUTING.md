# Contributing to `tack`

> [!WARNING]
> This file is a work-in-progress (WIP). If you have any questions, please open
> up an issue or email me directly at
> [jayshozie@gmail.com](mailto:jayshozie@gmail.com) to ask any questions until
> this file is considered done.

## 1. Getting Started

- **Prerequisites:** [README.md/Requirements](./README.md#requirements)
- **Build System:** GNU Make (with phony `make release` and `make debug`)

---

## 2. Coding Guidelines (***WIP***)

- **Coding-Style:** This project uses a [`.clang-format`](./.clang-format) file
to format the code. Please format your code with it before opening up a PR.
- **C Standard:** This project accepts C11 as its C standard. Please do not use
C23 features.
- **Multi-threading:** All database interactions must be thread-safe.
- **TUI:** Respect the `notcurses` terminal detection logic.

### Commit Messages

This project uses the Conventional Commits commit style with a bit of spice. You
can learn more about it at their [website](https://www.conventionalcommits.org/).

- General Structure:
```gitcommit
<type>(<scope>): <subject>
<BLANK LINE>
<justification>
<BLANK LINE>
<changes>
<BLANK LINE>
<footer>
<BLANK LINE>
```

- Example Commit:
```gitcommit
fix(tui)!: Fix memory leak caused by build_nav_pane

There is a memory leak caused in the first for loop of the build_nav_pane
function.

Fixes the memory leak in build_nav_panne by explicitly free'ing the items
array in a loop at the end of the function.

Fixes #1 #2 #3

Signed-off-by: Example J. Contributor <example@example.org>
```

#### Example Types:

1. **feat:** If your commit adds a feature, then use it.
2. **fix:** Used when the commit patches a bug in the codebase. If this commit
resolves open issues in GitHub, you must reference it in the footer using the
syntax `Fixes #<issue_number>` (placed above your `Signed-off-by` line). If not
you can ommit it. `fix` shouldn't be used for refactors or typo fixes and must
fix an actual bug in the current code. See refactor and style types.
3. **refactor:** If your commit changes how a functions works internally (e.g.
turning a while loop into a for loop for readability) without changing its
arguments and return values, or rename a variable for clarity, etc. then use
this. If your code fixes a bug, read the `fix` type; if it adds a feature, read
the `feat` type.
4. **perf:** If your commit only improves performance, use this.
5. **style:** If your commit uses this type, it cannot change how the code
actually works. It should only be used if you ran `clang-format` to fix
formatting issues, or fixed a missing space, a trailing comma, or change double
quotes to single quotes, etc.
6. **docs:** If your commit only touches the `docs/` directory, use this.
7. **test:** If your commit is adding/changing the tests (which don't exist
yet), use this.
8. **config:** If your commit changes stuff in the `.clang-formact` file,
`.gitignore` file, or adds a GitHub Actions YAML file, etc. then use this.

#### Currently Available Scopes:

1. **backend:** the commit touches the `src/backend` and/or `include/backend`
directories
2. **log:** the commit touches the `src/log` and/or `include/log` directories
3. **module:** the commit touches the `src/module` and/or `include/module`
directories
4. **state:** the commit touches the `src/state` and/or `include/state`
directories
5. **tui:** the commit touches the `src/tui` and/or `include/tui` directories
6. **main:** the commit touches the `src/main.c` file
7. **build:** the commit touches the `Makefile`
8. **meta:** the commit touches a file located at the root of the repository
(e.g. `.clang-format`, `compile_flags.txt`, `.gitignore`, etc.)

---

## 3. The Developer's Certificate of Origin (DCO)

To ensure the integrity of the codebase, `tack` uses the [DCO](./docs/DCO). By
adding a `Signed-off-by:` line to your commit message, you certify that you have
the right to submit the code under the project's [license](./LICENSE).

### How to Sign Your Work

Use the `-s` flag when committing.

```bash
# Either
$ git commit -s
# Or
$ git commit -s -m 'your commit message'
```
