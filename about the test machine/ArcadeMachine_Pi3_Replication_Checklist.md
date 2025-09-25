# Arcade Machine — Raspberry Pi 3 Replication Checklist (Pi3 Test Machine)

Below is a checklist of the steps we took to reproduce the current state on a Raspberry Pi, plus unfinished parts and suggestions for improvement. Follow these steps to replicate the environment and get the ArcadeMachine UI running.

---

## Replication Target (Current State)

On **Raspberry Pi OS** (kernel **6.1**, **g++ 10**), we built only the **SplashKitBackend** static library from the `splashkit-core` branch from early **2022**.

We used a **shim dynamic library** to package the Backend into the `libSplashKit.so` expected by the legacy project (exporting legacy API names like `open_database` / `run_sql` / `query_success`).

We also wrote a **header compatibility layer** (`sk-compat`) to map the `splashkit.h` included in the legacy project to the new Backend header/namespace, forwarding commonly used functions and macros so we can compile the **ArcadeMachine** main program and start the UI.

Running **ArcadeMachine** successfully enters the interface; audio and some games still need adaptation (see **Unfinished & Suggestions**).

---

## 1) Environmental Requirements

- **Device:** Raspberry Pi (armv7)  
- **OS:** Raspberry Pi OS (desktop, capable of running X11)  
- **Compiler:** g++ 10.2 (Raspbian)  

**Directory layout** (match this to reduce path differences):

```text
/opt/arcade/
  arcade-machine/      # Main program
  arcade-games/        # Game collections
  sk-compat/           # Compatibility headers
  sk-shim/             # Dynamic library wrapper
/opt/splashkit-core/   # 2022 version core repository
```

---

## 2) Get the Repositories

```bash
# 1) splashkit-core (reverted to around 2022-08)
sudo mkdir -p /opt && sudo chown -R "$USER":"$USER" /opt
git clone https://github.com/ZGT23/splashkit-core.git /opt/splashkit-core
cd /opt/splashkit-core

# Make sure you're in the 2022-08 commit (we used commit 2e75c14)
git show -s --date=short --pretty='commit %h %ad %s'

# 2) arcade-machine (your own fork)
git clone https://github.com/<yourname>/arcade-machine.git /opt/arcade/arcade-machine

# 3) arcade-games (your own fork)
git clone https://github.com/<yourname>/arcade-games.git /opt/arcade/arcade-games
```

---

## 3) Build the “legacy” SplashKitBackend (backend only)

```bash
cd /opt/splashkit-core
rm -rf build
cmake -S projects/cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j "$(nproc)" --target SplashKitBackend
sudo cmake --install build
sudo ldconfig
```

When completed, you should have:

```
/usr/local/lib/libSplashKitBackend.a
/usr/local/include/SplashKitBackend/*.h
```

We chose **not** to directly generate the official `libSplashKit.so`, but instead used the shim in the next step to export the legacy API symbols. This is the path we validated on the Pi.

---

## 4) Build the shim dynamic library
_Pack the Backend static library into `libSplashKit.so` and export legacy symbols._

```bash
mkdir -p /opt/arcade/sk-shim && cd /opt/arcade/sk-shim

# CMakeLists.txt
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.13)
project(SKShim CXX)

add_library(SplashKit SHARED empty.cpp)
target_include_directories(SplashKit PRIVATE /usr/local/include/SplashKitBackend)

# Pack the entire Backend static library to ensure symbols are exported
target_link_libraries(SplashKit PRIVATE -Wl,--whole-archive /usr/local/lib/libSplashKitBackend.a -Wl,--no-whole-archive)
EOF

# An empty source file
echo "extern \"C\" void __skshim_keep(){}" > empty.cpp

# Build & Install
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j "$(nproc)"
sudo install -m 755 build/libSplashKit.so /usr/local/lib/libSplashKit.so
sudo ldconfig

# Verify that legacy database-related symbols are exported (example)
nm -D /usr/local/lib/libSplashKit.so | grep -E 'open_database|run_sql|query_success|has_row|query_column_for_' | head
```

You should see exported symbols (mostly namespace-qualified symbol names, sufficient for linking).

---

## 5) Install the header compatibility layer (`sk-compat`)
_Resolves inclusion and call discrepancies in legacy code._

We adapted `splashkit.h` to forward to the Backend headers and provided shims for a few legacy functions/macros to avoid overload ambiguity (do **not** `using namespace splashkit_lib;` to prevent duplicate-name conflicts). Place this file at `/opt/arcade/sk-compat/include/splashkit.h`:

```cpp
#pragma once

// Only include the Backend header, avoid using-directives to reduce ambiguity
#include <string>
#include <vector>
#include <SplashKitBackend/utils.h>
#include <SplashKitBackend/graphics.h>
#include <SplashKitBackend/color.h>
#include <SplashKitBackend/text.h>
#include <SplashKitBackend/input.h>
#include <SplashKitBackend/keyboard_input.h>
#include <SplashKitBackend/window_manager.h>
#include <SplashKitBackend/sprites.h>
#include <SplashKitBackend/animations.h>
#include <SplashKitBackend/bundles.h>
#include <SplashKitBackend/music.h>
#include <SplashKitBackend/sound.h>
#include <SplashKitBackend/camera.h>
#include <SplashKitBackend/rectangle_drawing.h>
#include <SplashKitBackend/point_geometry.h>
#include <SplashKitBackend/geometry.h>
#include <SplashKitBackend/drawing_options.h>
#include <SplashKitBackend/types.h>

// -- Type short names: Export commonly used types to the global scope -- //
using splashkit_lib::sprite;
using splashkit_lib::bitmap;
using splashkit_lib::point_2d;
using splashkit_lib::rectangle;
using splashkit_lib::vector_2d;
using splashkit_lib::color;
using splashkit_lib::key_code;

// -- Color macros maintain historical conventions -- //
#define COLOR_BLACK (splashkit_lib::color_black())
#define COLOR_WHITE (splashkit_lib::color_white())

// -- "Unambiguous forwarding" of commonly used functions -- //
inline void refresh_screen() { splashkit_lib::refresh_screen(); }
inline void refresh_screen(unsigned int /*fps*/) { splashkit_lib::refresh_screen(); }

inline void draw_text(const std::string &t, const color &c, const std::string &font, int sz, double x, double y) {
  splashkit_lib::draw_text(t, c, font, sz, x, y);
}
inline void clear_screen(const color &c) { splashkit_lib::clear_screen(c); }
inline void process_events() { splashkit_lib::process_events(); }
inline bool quit_requested() { return splashkit_lib::quit_requested(); }
inline bool key_down(key_code k){ return splashkit_lib::key_down(k); }
inline bool key_typed(key_code k){ return splashkit_lib::key_typed(k); }

inline void play_music(const std::string &name){ splashkit_lib::play_music(name); }
inline void stop_music(){ splashkit_lib::stop_music(); }
inline bool music_playing(){ return splashkit_lib::music_playing(); }

inline void write_line(const std::string &s){ splashkit_lib::write_line(s); }

// Old resource pack API mapping
inline void load_resource_bundle(const std::string &name, const std::string &path){ 
  splashkit_lib::load_resource_bundle(name, path);
}
inline void free_resource_bundle(const std::string &name){ 
  splashkit_lib::free_resource_bundle(name);
}

// sprites / camera commonly used
using splashkit_lib::sprite_center_point;
using splashkit_lib::sprite_position;
using splashkit_lib::sprite_set_position;
using splashkit_lib::sprite_set_dx;
using splashkit_lib::sprite_set_dy;
using splashkit_lib::create_sprite;
using splashkit_lib::bitmap_named;
using splashkit_lib::music_named;
using splashkit_lib::draw_bitmap;
using splashkit_lib::update_animation;
using splashkit_lib::animation_ended;

using splashkit_lib::screen_width;
using splashkit_lib::screen_height;
using splashkit_lib::to_screen;
using splashkit_lib::to_screen_x;
using splashkit_lib::to_screen_y;
using splashkit_lib::point_on_screen;
using splashkit_lib::rect_on_screen;
using splashkit_lib::camera_x;
using splashkit_lib::camera_y;
using splashkit_lib::move_camera_to;
using splashkit_lib::set_camera_x;
using splashkit_lib::set_camera_y;

// window related
using splashkit_lib::open_window;
using splashkit_lib::hide_mouse;
using splashkit_lib::window_toggle_border;
using splashkit_lib::window_toggle_fullscreen;
using splashkit_lib::window_is_fullscreen;
```

> **Notes**
>
> - We wrap the “ambiguous” functions with `inline` and avoid broad `using` to reduce naming conflicts.  
> - The `refresh_screen(unsigned int)` overload is key: it swallows old calls like `refresh_screen(60)` and forwards to the parameterless version, avoiding the “overload ambiguity” compile error.  
> - If more functions/enums are missing, add similar inline forwarders here.

---

## 6) Compile ArcadeMachine

```bash
cd /opt/arcade/arcade-machine
make clean 2>/dev/null || true

# Key: Include sk-compat headers and link against libSplashKit.so
make \
  CPPFLAGS="-I/opt/arcade/sk-compat/include -I/usr/local/include/SplashKitBackend" \
  CXXFLAGS="-Iinclude -I/opt/arcade/sk-compat/include -I/usr/local/include/SplashKitBackend" \
  LDFLAGS="-lstdc++fs -lSplashKit"
```

Successful compilation should produce `./ArcadeMachine` and:

```bash
ldd ./ArcadeMachine | grep SplashKit
# Should show: /usr/local/lib/libSplashKit.so
```

---

## 7) Run ArcadeMachine (runtime environment)

Run inside a **graphical desktop session** (not pure SSH):

```bash
cd /opt/arcade/arcade-machine

# If the audio device is missing, use the "silent driver" to avoid errors.
export SDL_AUDIODRIVER=dummy
./ArcadeMachine
```

**Common symptoms & fixes**

- **Font/Audio warnings:** Ignore when using the dummy audio driver. Ensure `resources/fonts/*.ttf` exist and are included in the resource bundle.  
- **Menu freeze on first entry:** Likely due to `arcade-games` cloning/indexing or missing resources—clone `/opt/arcade/arcade-games` beforehand and confirm resources are in place.

---

## 8) Compile and run a C++ game (example: _BelowTheSurface_)

This repository may not ship with a Makefile (in our version). You can compile quickly with one command:

```bash
cd /opt/arcade/arcade-games/games/BelowTheSurface

# Single-file example (program.cpp with include/ )
g++ -std=gnu++14 \
  -I include \
  -I /opt/arcade/sk-compat/include \
  -I /usr/local/include/SplashKitBackend \
  program.cpp \
  -lSplashKit -lstdc++fs -O3 \
  -o below_the_surface

# Run (dummy audio still recommended)
export SDL_AUDIODRIVER=dummy
./below_the_surface
```

If there are multiple `.cpp` files, append them to the command, or create a minimal Makefile with the same include path and `-lSplashKit` link.

---

## 9) Targeted fixes we applied

- **`refresh_screen(60)` overload ambiguity:** Provided `inline void refresh_screen(unsigned int)` in `sk-compat/splashkit.h`.  
- **Missing `sprite_center_point` / `music_playing` symbols:** Resolved via `using` or inline forwarders to `splashkit_lib::...` in `sk-compat`. Add more as needed.  
- **Database symbols missing at link:** Pack `libSplashKitBackend.a` via shim `libSplashKit.so` to satisfy old-style symbols.  
- **No audio device:** `export SDL_AUDIODRIVER=dummy` to avoid “Attempting to load music when audio is closed”.

---

## 10) Unfinished & Constructive Suggestions

### A. Uniform `sk-compat` mapping
Systematically review the legacy API and provide items one by one in `sk-compat/splashkit.h`:

- `write_line` / `delay` / timers family  
- Overloads for bundles (e.g., `string_view`)  
- Keep exports focused on **types & constants**; use **inline forwarders** for functions to avoid conflicts  
- Consider a separate repo/subdir for `sk-compat` and include it in build-product testing

### B. Provide a common Makefile template for `arcade-games`

Place a minimal `Makefile` in each game directory so ArcadeMachine can run `make -C <game>` before launching (avoids a blank screen):

```make
CXX := g++
CXXFLAGS := -std=gnu++14 -O3 -I include -I /opt/arcade/sk-compat/include -I /usr/local/include/SplashKitBackend
LDFLAGS := -lSplashKit -lstdc++fs

SRC := $(wildcard *.cpp src/*.cpp)
BIN := game

all: $(BIN)

$(BIN): $(SRC)
\t$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

clean:
\trm -f $(BIN)
```

### C. Robustness of resource bundles

- Verify resource files exist **before** launching a game; show a clear UI prompt if missing instead of a blank screen.  
- On silent devices, failures in `play_music`/`load_sound_effect` should warn but **not** interrupt the process.

### D. Window/Display environment detection

We encountered: `displayIndex must be in the range 0 - -1` — happens when there is **no desktop session** or `DISPLAY` is unset. Detect `SDL_VIDEODRIVER` / `DISPLAY` at launch and provide a helpful prompt in SSH environments (or fallback to dummy video).

### E. Script the shim build

Automate the three-step CMake build (`scripts/build-shim.sh`) for one-click install to lower entry barriers.

### F. CI verification

Set up a minimal GitHub Action (for ARM via cross/runner) to run header and link checks to avoid future breakages after system upgrades.

---

_This Markdown was adapted directly from the original operation manual for the Pi3 test machine to preserve readability while making it easy to follow and copy-paste commands._
