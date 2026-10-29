# AGENTS.md

Scaffold for Linux driver work on **Nokia** hardware, hosted as
`Notempty-long/nokia-drivers`.

## Current state: empty

- **No source, no build system, no tests, no CI.** `AGENTS.md` is the only file; the
  rest is four empty directories. Do not invent `make` / test / lint commands — there
  is nothing to run.
- Don't describe this repo as containing drivers, firmware, or a port in progress.

## Directory intent

Bucket names are the entire spec. Nothing has been added to confirm them:

- `Wi-Fi/` — Wi-Fi driver and/or firmware blobs
- `PMIC/` — power management IC (charger, regulators)
- `audio/` — audio codec / ALSA work
- `touch/` — touchscreen driver

The target SoC and device model are **not recorded anywhere**. Establish them before
writing driver code: they decide the vendor kernel tree, which PMIC is involved, and
whether mainline drivers already cover the hardware.

## Environment gotchas

- Host is **Windows**, and `wsl`, `docker`, `make`, `gcc`, `clang` are all absent.
  Kernel modules **cannot be compiled or loaded here** — say so instead of guessing.
- Real work needs Linux: WSL2 or a VM with headers under
  `/lib/modules/$(uname -r)/build`.
- `.gitignore` covers `platform-tools/` and `*.zip` only. Add kernel build output
  (`*.ko`, `*.o`, `*.mod*`, `Module.symvers`) to it before building anything.
- `adb`/`fastboot` are available at `./platform-tools/` (gitignored, never commit
  them). Use `./platform-tools/adb.exe`, it is not on `PATH`.

## Git

- **Every change needs a commit and a pull request, however small.** Do not leave work
  sitting uncommitted in the working tree, and do not treat a one-line edit as too
  minor to commit.
- `origin` → `https://github.com/Notempty-long/nokia-drivers`, branch `main`, already
  configured as upstream, so plain `git push` works (no `-u` needed).
- HTTPS auth uses Git Credential Manager, already set system-wide
  (`credential.helper=manager`). Pushing still requires being the owner or a
  collaborator of that repo.
- **`user.name` / `user.email` are not configured.** Commits fail with
  "Author identity unknown" until set.
- Empty directories are not tracked: add `.gitkeep` when committing the structure.
