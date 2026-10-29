# AGENTS.md

Scaffold for Linux driver work on **Nokia** hardware, hosted as
`Notempty-long/nokia-drivers`.

## Current state: four minimal modules, no working driver

- **No tests, no CI, and nothing has ever been compiled.** `make` has not been run
  on this work; do not claim the modules build.
- What is tracked: `AGENTS.md`, `.gitignore`, `the devicetree.dts`, a `Makefile`,
  one `README.md` + one `.c` + one `Kbuild` per subsystem directory.
- The modules are deliberately inert bring-up probes:

  | Module | Binds hardware | Loads firmware |
  |---|---|---|
  | `touch/hx83102_fw.c` | yes, SPI `mediatek,himax_cap_touch` | yes, `Himax_firmware.bin` |
  | `Wi-Fi/mt6631_fw.c` | no (plain module) | yes, 5 MT6631 blobs |
  | `PMIC/mt6357_stub.c` | no | no — block has no firmware |
  | `audio/mt6357_codec_stub.c` | no | no — codec has no firmware |

  Everything past firmware loading is stubbed with `TODO:` markers. These are not
  drivers and must not be described as a port.
- `touch/hx83102_fw.c` claims the same `mediatek,himax_cap_touch` compatible as the
  in-tree `hxchipset` driver. Never load both.
- `the devicetree.dts` is a **decompiled** DTB (numeric phandles, no labels) from the
  reference BSP, **not** from this phone. The live device tree differs: it has a
  `spi2@11012000/fts_touch@0` child and a populated `/touch` node, both absent from
  this file. Read it for reference only — it cannot be edited and rebuilt.

## Target device (established)

- **Nokia 2.3**, retail model `TA-1206`, codename `Ironman_00WW` / `IRM_sprout`.
- SoC **MediaTek MT6761** (Helio A22), device-tree model string `MT6761V/WBB`.
- Android 11, build `00WW_3_31J` / `00WW_3_310_SP09`, board id `T99621AA1_4`.
- Reference BSP tree (not the phone's tree, but the same driver code) lives at
  `C:\n23\src\LINUX\android\kernel` — Linux 4.19.127, `CONFIG_MTK_PLATFORM="mt6761"`,
  defconfig `k61v1_64_bsp_defconfig`.
- Serial `0123456789ABCDEF`. **Root works via Magisk** (`adb shell su -c '...'`);
  `adb root` does not (production build). `/proc/device-tree` and `modalias` are
  SELinux-blocked for the `shell` user and need `su -c`.

## Directory intent

- `Wi-Fi/` — MT6631 combo (gen4m). Driver source is **not** in the BSP.
- `PMIC/` — MT6357 plus an external **ETA6937** charger IC.
- `audio/` — MT6357 codec on the shared `common_int` machine driver (`mt-snd-card`).
- `touch/` — Himax **HX83102D** TDDI over **SPI2, CS0, 10 MHz**, INT on GPIO 0.

Each directory's `README.md` carries the evidence and the open questions. Read the
relevant one before writing code.

## Environment gotchas

- Host is **Windows**, and `wsl`, `docker`, `make`, `gcc`, `clang` are all absent.
  Kernel modules **cannot be compiled or loaded here** — say so instead of guessing.
  The top-level `Makefile` needs Linux and has never been run.
- Real work needs Linux: WSL2 or a VM with headers under
  `/lib/modules/$(uname -r)/build`.
- `.gitignore` covers `platform-tools/`, `*.zip` and kbuild output (`*.ko`, `*.o`,
  `*.mod*`, `.*.cmd`, `Module.symvers`, `modules.order`).
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
