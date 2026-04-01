# Arcade Machine — Engineering Handoff Document

> Last updated: 2026-04-01 (observability work in progress)  
> Original session: Claude Sonnet 4.6 via VS Code extension  
> Recipient: Next AI assistant (ChatGPT Codex or new Claude session)

---

## Project Overview

C++ arcade machine frontend built on the **SplashKit** game framework (SDL2-based).  
Runs on a **Raspberry Pi 3B** (Linux ARM), displays a carousel game menu on a TV,  
launches games as child processes, tracks stats via SQLite.

**Working directory on RPi:** `~/arcade-machine/`  
**Games directory:** `~/arcade-machine/games/games/`  
**Connection:** SSH from developer laptop

---

## Architecture Summary

```
program.cpp
  └── ArcadeMachine
        ├── Menu          — carousel UI, game launching (src/Menu.cpp)
        │     └── ButtonNode  — circular doubly-linked list of game buttons
        ├── Option        — settings screen (src/Option.cpp)
        ├── AboutScreen   — about/credits screen
        └── Rating        — star rating system

Game discovery chain:
  Helper::ConfigDataList()
    → getConfigFiles("./games/games")     [scans for config.txt files]
    → ConfigData(configFile)              [parses each config.txt]
    → Menu::createButtons()              [loads images, creates sprites]

Game launch chain (Linux):
  Menu::carouselHandler()
    → ConfigData::getExecutablePath()    [finds binary, two paths — see below]
    → Menu::startGame(path)
    → spawnProcess(dir, file)            [fork() + popen()]
    → Menu::checkGameExit()              [polls processRunning() each frame]
```

### Key files

| File | Role |
|------|------|
| `src/program.cpp` | Entry point |
| `src/ArcadeMachine.cpp` | Top-level controller |
| `src/Menu.cpp` | Carousel menu + game launching |
| `src/Process.cpp` | Linux fork/popen process management |
| `src/ConfigData.cpp` | config.txt parser + git clone/pull |
| `src/Option.cpp` | Options/settings screen |
| `include/Helper.h` | Game discovery, filesystem scanning |
| `include/Rating.h` | Star rating value object |
| `resources/bundles/resources.txt` | SplashKit resource bundle manifest |

### Platform macros (defined in Configuration.h)

- `ARCADE_MACHINE_OS` — `"linux"` / `"windows"` / `"macos"`
- `ARCADE_MACHINE_INSTRUCTION_SET` — `"arm"` / `"aarch64"` / `"x86_64"`
- `ARCADE_MACHINE_BINARY_EXT` — `""` (Linux/Mac) / `".exe"` (Windows)
- `ARCADE_MACHINE_PATH_SEP` — `"/"` (Linux/Mac) / `"\\"` (Windows)

---

## Game Executable Resolution (`ConfigData::getExecutablePath`)

Two-path fallback logic in `src/ConfigData.cpp:233`:

**Path 1 — Auto build (preferred):**  
Looks for `{game_folder}/builds/linux-arm` (or `linux-aarch64`, `windows-x86_64.exe`, etc.)  
No config.txt changes needed; file must exist with correct naming convention.

**Path 2 — Manual config (fallback):**  
Reads `lin-exe=` field from `config.txt`.  
Throws `std::runtime_error` if empty or file not found.  
Exception IS caught in `Menu.cpp` and printed via `write_line()`.

**Current state:** All 9 games have empty `Linux Bin` — neither path works for any game.  
This is a content problem (games not compiled for Linux/ARM), not a code bug.

---

## Current Task Priority

### Priority 1 — Observability (IN PROGRESS)

All output uses `std::cerr << "[Module] message" << std::endl` format.  
`std::endl` flushes immediately — safe even if program crashes mid-run.  
Visible in SSH terminal when running `DISPLAY=:0 ./ArcadeMachine`.

#### DONE

**`src/Process.cpp`** — fully instrumented:
- `fork()` returns -1 → `[Process] fork failed: <strerror>`
- Parent after successful fork → `[Process] spawned game: <dir>/<file> (PID: <pid>)`
- `chdir()` fails → `[Process] chdir failed: <directory>` then exits child
- `popen()` fails → `[Process] popen failed: <cmd>` then exits child

**`src/Menu.cpp` — `createButtons()`** — image loading instrumented:
- `image=` field empty in config.txt → `[Menu] image load failed (empty path): <title>`
- File not found on disk → `[Menu] image load failed (file not found): <path>`
- SplashKit failed to load → `[Menu] image load failed (splashkit error): <path>`
- Success → `[Menu] image loaded: <path>`

**`src/Menu.cpp` — `startGame()` + `drawMenuPage()` + `checkGameExit()`** — game launch全流程:
- 启动前 → `[Menu] starting game: <file> from <path>`
- fork失败 → `[Menu] failed to start game: fork failed` + 屏幕红色错误3秒
- fork成功 → `[Menu] game process spawned (PID: <pid>)` + 屏幕动态"Starting <title>..."
- 游戏3秒内退出 → `[Menu] game crashed on startup` + 屏幕红色崩溃信息3秒
- 正常退出 → `[Menu] game process ended: <title> (PID: <pid>)`

**`src/ConfigData.cpp` — `getFromGit()`** — git 操作全程有输出:
- 开始前 → `[Git] cloning/pulling: <url> → <dir>`
- 结束后 → `[Git] clone/pull succeeded/failed: <dir>`

**`include/Helper.h` — `getConfigFiles()`** — 扫描结果:
- 扫描结束 → `[Helper] found N config files in <dir>`

**`src/ConfigData.cpp` — `ConfigData` 构造函数** — 关键字段缺失检查:
- `title` 为空 → `[Config] missing title: <config_path>`
- `image` 为空 → `[Config] missing image: <config_path>`
- `linux-bin` 为空 → `[Config] missing linux-bin: <config_path>`

**`src/program.cpp`** — 启动资源检查:
- `resources.txt` 不存在 → `[Bundle] resources.txt not found`
- 资源包加载结果 → `[Bundle] resource bundle loaded/failed`
- 逐个检查所有 bitmap 和 font → `[Bundle] bitmap not loaded: <name>`
- 汇总 → `[Bundle] N resource(s) failed to load`

**`src/ArcadeMachine.cpp` — `playSplashKitIntro()`** — git 失败不再死循环:
- 最多重试3次，每次等待30秒
- 每次在屏幕和终端输出当前是第几次尝试
- 3次全失败 → 屏幕显示错误5秒后 `exit(EXIT_FAILURE)`

**`src/ConfigData.cpp` — `openFile()`** — 报错补充文件路径:
- `[Config] failed to open file: <path>`

#### PENDING

无。Priority 1 可观测性工作全部完成。

### Priority 2 — Bug fixes (DONE — 16 bugs fixed, not yet deployed to RPi)
See section below.

### Priority 3 — Game loading
Games need compiled Linux ARM binaries. Either:
- Pre-compile and place in `{game_folder}/builds/linux-arm` (or `linux-aarch64`)
- Or add `lin-exe=` to each game's `config.txt`

Some games reportedly have pre-built versions; needs investigation of actual directory structure.

---

## Completed Bug Fixes (not yet synced to RPi)

All fixes are in the working directory on the developer's laptop.  
**None of these have been deployed to the RPi yet.**

| # | File | Problem | Fix |
|---|------|---------|-----|
| 1 | `include/Helper.h` | `getFolderName()` returned trailing slash → double-slash paths | Strip trailing `/` or `\` |
| 2 | `include/Helper.h` | `getConfigFiles()` no existence check → crash if `./games/games` missing | Add `fs::exists()` guard |
| 3 | `src/ConfigData.cpp` | Hardcoded `#include <experimental/filesystem>` → compile failure on C++17 | Conditional include |
| 4 | `src/ConfigData.cpp` | `getFromGit()` always returned `true` even on git failure | Check directory exists after clone |
| 5 | `src/Menu.cpp` | `createButtons()` called `create_sprite` without prior `load_bitmap` | Add `load_bitmap(image, image)` before `create_sprite` |
| 6 | `src/Menu.cpp` | Windows launch: dangling pointer from `.c_str()` on temporary string | Use persistent `std::string` + `std::vector<char>` |
| 7 | `src/Menu.cpp` | Windows launch: duplicate pre-fade variable assignment | Remove duplicate line |
| 8 | `src/Menu.cpp` | Both constructors: `m_tip` uninitialized | Add `m_tip = nullptr` |
| 9 | `src/Menu.cpp` | `drawMenuPage()`: unconditional `m_tip->draw()` → null dereference | Guard with `if (!m_inGame && m_tip)` |
| 10 | `src/Menu.cpp` | Destructor: circular `ButtonNode` list never freed → memory leak | Add full list traversal and delete |
| 11 | `include/Menu.h` | Dead `LPCSTR m_gamePath`, `LPSTR m_gameExe`, `LPCSTR m_gameDir` members | Remove |
| 12 | `src/Option.cpp` | `y_pos` uninitialized → undefined behaviour | Add `= 0` |
| 13 | `src/Option.cpp` | Missing destructor → `ButtonNode` list leak | Add `~Option()` with list cleanup |
| 14 | `include/Option.h` | Circular `#include "ArcadeMachine.h"` (unused) | Remove |
| 15 | `src/program.cpp` | `ArcadeMachine` constructed before `open_window()` → `create_sprite` called without render context | Move `open_window` + `window_toggle_border` before constructor |
| 16 | `resources/bundles/resources.txt` | 4 music entries had leading spaces in names → SplashKit lookup fails | Remove spaces: `MUSIC, 1, 1.mp3` → `MUSIC,1,1.mp3` |

---

## Known Issues / Not Yet Fixed

### All games — No Linux executables configured
All 9 games have empty `Linux Bin` in their config.txt.  
`getExecutablePath()` throws `std::runtime_error` which IS caught and printed via `write_line()`.  
Games with pre-built binaries need path configured or binaries moved to `builds/linux-arm` (or `linux-aarch64`).

---

## Deployment Notes

Changes are on developer laptop only. To deploy to RPi:
```bash
# Option A: rsync
rsync -av --exclude='.git' ~/path/to/arcade-machine/ pi@192.168.x.x:~/arcade-machine/

# Option B: git commit + push + pull on RPi
```
RPi is accessed via SSH. Program launched with:
```bash
DISPLAY=:0 ./ArcadeMachine
```

---

## Collaboration Style Notes

- User has C++ basics, not deeply familiar with this codebase
- Teach concepts as we go, don't just silently fix everything
- One-shot fixes preferred — find ALL issues in a file before reporting
- RPi is physical hardware; can't iterate quickly → get fixes right before deploying
- Current session partner: Claude Sonnet 4.6 (VS Code extension)
