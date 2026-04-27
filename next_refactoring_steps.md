# Next Refactoring Steps — matrixserver

Trimmed copy of the original refactoring plan (`/Users/bjoern/.claude/plans/you-are-a-c-parallel-harp.md`)
with all implemented items removed. What remains: items that were deferred, partially
implemented, or explicitly skipped during the 10-commit refactor on `more-refactoring`
(commits `83aa63c` … `7618c4e`). Plus a section at the end for items discovered while
executing the plan.

Each open item carries:
- **Status** — Deferred / Partial / Skipped, plus the specific reason.
- The **task-specific information from the original plan**, retained verbatim where
  useful, so a subagent can act on it cold.

For build / verification flow and the LC_RPATH gotcha, see the "Build & verification
setup" section at the bottom — copied unchanged from the original plan.

---

## Progress Summary (updated 2026-04-27)

| Item | Status | Commit | Notes |
|------|--------|--------|-------|
| **P0** — boost::thread → std::thread | ✅ DONE | `12599a1` | All 4 remaining classes migrated |
| **S2** — LoopRunner extraction | ✅ DONE | `12599a1` | Path A composition; legacy mirror remains |
| **S4** — Runtime renderer selection | ⏳ DEFERRED | — | Needs CI/Docker/Debian coordination |
| **S7** — Connection abstraction | ⏳ PARTIAL | `7618c4e` | Buffers modernized; Unix socket audit open |
| **C1.a** — enum class ScreenNumber | ⏳ DEFERRED | — | Needs example-apps PR |
| **C1.b** — std::span in IRenderer | ⏳ DEFERRED | — | Needs C++20 bump |
| **C1.c** — std::optional lookups | ❌ SKIP | — | No clarity win; leave as-is |
| **C2** — Error handling policy | ⏳ DEFERRED | — | Needs design discussion |
| **C3** — Naming consistency | 🔧 PARTIAL | `12599a1` | Config files done; formatting commit pending |
| **C4** — Config consolidation | ⏳ DEFERRED | — | Needs protobuf schema changes |
| **C5** — Unit tests | ✅ DONE | `12599a1` | Dispatcher, ConnectionFactory, InputState tests |
| **N1** — RendererRegistry | ✅ DONE | `12599a1` | Extracted from Server |
| **N3** — LC_RPATH fix | ⏳ OPEN | — | Lives in example-apps repo |

**Next actionable items** (no coordination gates):
1. **C3 formatting commit** — Apply .clang-format to codebase, commit, update .git-blame-ignore-revs
2. **S2 legacy mirror removal** — Migrate MatrixRain to InputState accessors, then remove dual-write
3. **C2 quick wins** — Add logging to silent try_lock failures in FPGA renderers

---

## Context

The matrixserver project at `matrixserver/` started as a proof-of-concept LED-cube
rendering server and has grown organically into a multi-backend system (FPGA-FTDI,
FPGA-RPISPI, RGB-Matrix, WebSocket simulator) with a shared application library, a
server daemon, and several example apps. The first 10 refactor commits eliminated raw
thread/memory ownership, fixed a cross-thread mutex deadlock, decomposed
`MatrixApplication` and `Server` into named units (ConnectionFactory,
MessageDispatcher, InputState, DefaultAppLauncher, ServerBootstrap, RendererFactory),
modernized the connection-buffer storage, split `IRenderer` along the messaging axis,
and replaced the `ServerSetup` nested switches with a layout table.

**Commit `12599a1` (2026-04-27)** completed P0, S2 (LoopRunner), N1, C3 (config), C5
(tests), and fixed the pre-existing protobuf duplicate descriptor crash in the test
binary.

The items below are what remains to reach the original exit criteria and to clean up
follow-on work surfaced by the executed refactors.

---

## Priority Issues

### P0 — Migrate `boost::thread` → `std::thread` *(DONE)*

**Status:** ✅ DONE — Commit `12599a1`. All four remaining classes migrated:
- `application/Mpu6050.{h,cpp}` — `boost::mutex` → `std::mutex`, added `running_` atomic + destructor
- `application/ADS1000.{h,cpp}` — same pattern
- `application/MatrixApplicationStandalone.{h,cpp}` — two threads migrated (mainRunning, renderRunning)
- `renderer/WebSocketSimulatorRenderer/WebSocketSimulatorRenderer.{h,cpp}` — thread + atomic
- `Boost::thread` dropped from all CMakeLists.txt (application/, common/, server/, renderer/WebSocketSimulatorRenderer/)

All 115 tests pass (4442 assertions). **Manual smoke-test needed on RPi** for Mpu6050/ADS1000.

**Note:** `threadLock_` members in Mpu6050 and ADS1000 are declared but never used. Could be removed in a follow-up cleanup.

---

## Structural Improvements

### S2 — Decompose `MatrixApplication` (SRP) *(Partial — `LoopRunner` DONE, legacy mirror remains)*

**Status:** ✅ `LoopRunner` DONE — Commit `12599a1`. Path A (composition) implemented:
- New `application/LoopRunner.{h,cpp}` — owns thread, FPS regulation, state machine, screen-access condvar, load tracking
- `MatrixApplication` holds `LoopRunner runner_` member
- `internalLoop()` removed; `start()` delegates to `runner_.start([this]{ return loop(); }, [this]{ renderToScreens(); checkConnection(); })`
- Public API preserved (constructor, `loop()`, `getFps()`, `getAppState()`, etc.)
- `LoopRunner.h` added to PUBLIC_HEADER in CMakeLists.txt

**Remaining:** Legacy IMU/audio mirror still exists (requires coordinated example-apps PR):
- `static float latestSimulatorImuX/Y/Z` etc. in `MatrixApplication.h`
- `static float latestSimulatorGyroX/Y/Z` etc. in `MatrixApplication.h`
- `static std::mutex simulatorImuMutex` in `MatrixApplication.h`
- `static uint8_t latestAudioVolume` in `MatrixApplication.h`
- `static std::vector<uint8_t> latestAudioFrequencies` in `MatrixApplication.h`
- `static std::mutex audioDataMutex` in `MatrixApplication.h`
- Dual-write logic in `InputState.cpp`

**Only known reader:** `LED_Cube-Example_Applications/MatrixRain/matrixrain.cpp:69-71`

**To complete the mirror removal (requires coordinated example-apps PR):**
1. Migrate `MatrixRain` to use `InputState` accessors instead of static fields
2. Remove the 6 static fields from `application/MatrixApplication.h`
3. Remove the static definitions at top of `application/MatrixApplication.cpp`
4. Remove dual-write logic in `application/InputState.cpp`
5. `application/InputState.h` can drop `MatrixApplication.h`-related includes

---

### S4 — Runtime renderer selection (OCP) *(Partial)*

**Status:** Partial — Phase 4b extracted `ServerBootstrap` and a renderer-factory
parameter. Both binaries (`matrix_server_simulator`, `matrix_server`) now share
bootstrap code. The single-binary-with-`--backend=`-flag part of the original plan
was **not** implemented.

**Why deferred:** Two-binary deployment is referenced in:
- `.github/workflows/*.yml` (build-and-test, package-and-push jobs)
- `Dockerfile`, `Dockerfile.arm64`, `Dockerfile.release`, `Dockerfile.arm64.release`
- `entrypoint.sh`
- `docker-compose.yml`
- both READMEs
- the Debian package metadata

Doing this without coordinating those is a regression risk; out of scope for a
matrixserver-only PR.

**Task-specific info (from original plan):**

Where: `server_hardware/main.cpp:14-27` selects renderer with `#if defined(BACKEND_*)`.
One executable per backend.

Refactor: Single executable with a renderer factory keyed off config or `--backend=`
CLI flag:
```cpp
std::shared_ptr<IRenderer> makeRenderer(const Config& cfg, screens);
```
Each backend's CMake target can be conditionally compiled (so a Pi-only build still
excludes FPGA-FTDI), but the *selection logic* is config-driven, not preprocessor-driven.

**Implementation outline (post-Phase-4b state):**
- Single executable target `matrix_server`.
- Each backend lib's `target_compile_definitions` adds `HAVE_FPGA_FTDI` /
  `HAVE_FPGA_RPISPI` / `HAVE_RGB_MATRIX` only when the lib is built (still gated by
  `-DHARDWARE_BACKEND=…`, which can become a list).
- New CLI flag `--backend=simulator|fpga-ftdi|fpga-rpispi|rgb-matrix`. Default
  `simulator`. Reject unknown values.
- The factory passed to `ServerBootstrap::runServer()` is selected by the flag,
  returning nullptr for `simulator`.
- Update CI / Dockerfiles / docs in lockstep.

---

### S7 — Consolidate connection abstraction *(Partial)*

**Status:** Partial — Phase 5d (7618c4e) replaced the raw `char[]` buffers in
`IpcServer`, `IpcConnection`, and `SocketConnection` with `std::vector<uint8_t>`. The
broader "single abstract MessageTransport with a shared framing layer" wording from
the original plan was not pursued because `UniversalConnection` already plays that
role; the concrete remaining work was the buffer modernization.

**Why partially skipped:** The `UniversalConnection` interface already exists and is
already used uniformly by callers. Introducing a new `MessageTransport` would be
parallel structure on top of an existing abstraction, with no new behaviour. Bringing
`UnixSocketClient/Server` to feature-parity with `SocketConnection` (COBS framing
applied consistently) is an open question worth doing if Unix sockets ever see
production use.

**Task-specific info (from original plan):**

Where: Three transport types (`SocketConnection`, `IpcConnection`,
`UnixSocketClient/Server`) overlap heavily but each has its own buffer scheme.

Refactor: A single abstract `MessageTransport` with a shared framing layer (COBS for
stream, atomic for IPC). Each concrete transport implements `read_one()` /
`write_one()` returning `std::vector<uint8_t>` or `std::span`. Eliminates per-class
buffer arithmetic.

**Concrete remaining work, if pursued:**
- Audit `common/UnixSocketClient.{h,cpp}` and `common/UnixSocketServer.{h,cpp}` for
  framing consistency with `SocketConnection` (COBS).
- Consider whether `UniversalConnection` should expose `read_one() / write_one()` as
  pure-virtual to enforce the framing contract.
- Skip if Unix sockets remain unused in production — current state works.

---

## Code Quality Improvements

### C1 — Modern C++ idioms across the board *(Partial — three sub-items remain)*

**Status:** Partial. Phase 2 (3dca550) did the mechanical sweep: NULL→nullptr,
`override` on concrete renderer virtuals, `#define`-constants → `inline constexpr`,
range-for `const auto&` fixes. Three sub-items were skipped at the time:

#### C1.a — `enum class` for `ScreenNumber` / `EdgeNumber` / `CornerNumber`

**Status:** Skipped during Phase 2.

**Why skipped:** Example apps (out of scope) use unqualified enumerator names
(`front`, `back`, `top`, …) and cast `int` → `ScreenNumber` directly. Migration would
break every example app at the source level.

**Task-specific info (from original plan):**
Convert `enum ScreenNumber/EdgeNumber/CornerNumber` in
`application/CubeApplication.h:54-56` to `enum class`.

**To complete (paired with example-apps PR):**
- In matrixserver: `enum ScreenNumber {…}` → `enum class ScreenNumber : int {…}`.
- In example apps: prefix usages with `ScreenNumber::front` etc., or `using enum
  ScreenNumber;` (C++20).
- Same for `EdgeNumber`, `CornerNumber`.

#### C1.b — `std::span<const Color>` in `IRenderer::setScreenData`

**Status:** Skipped during Phase 2 and Phase 5c (S1).

**Why skipped:** Project is C++17. `std::span` requires C++20.

**Task-specific info (from original plan):**
Use `std::span<const Color>` in `IRenderer::setScreenData` instead of raw `Color *`.

**To complete (after a C++20 bump):**
- `IRenderer::setScreenData(int, Color*)` → `setScreenData(int, std::span<const Color>)`.
- Update every implementation (FPGA*, RGBMatrix, WebSocketSimulator) and every
  caller (`Server.cpp` `handleSetScreenFrame`).
- The C++20 bump itself is a separate concern — toolchain audit needed (gcc 10+,
  Xcode current, Boost ABI compatibility on RPi targets).

#### C1.c — `std::optional` for failable lookups

**Status:** Skipped during Phase 2.

**Why skipped:** Empty `std::shared_ptr` already gives nullptr semantics for
`Server::getAppByID` (post-Phase-1) and the `*Client::connect` family. No clarity
win from converting to `std::optional<std::shared_ptr<…>>`.

**Task-specific info (from original plan):**
Use `std::optional` for failable lookups (`getAppByID`, `TcpClient::connect`).

**Recommendation:** Leave as-is unless a non-pointer-returning function is identified
where `std::optional` would clearly improve clarity.

---

### C2 — Consistent error handling policy *(Not done)*

**Status:** Not done.

**Why deferred:** Cross-cutting policy decision, not a single-PR refactor. Needs a
short design discussion ("which boundary throws vs. returns vs. logs?") before code
changes — applying the policy without alignment risks masking or surfacing latent
bugs.

**Task-specific info (from original plan):**

Symptom: Mix of exceptions, return-`bool`, raw-pointer-or-null, and silent
mutex-`try_lock` failures (e.g.
`renderer/FPGAFTDIRenderer/FPGARendererFTDI.cpp:43-48`).

Decision needed: pick one. Recommendation:
- Constructors / hard-fail init → exceptions.
- Frame-path / hot-loop → `bool` or `tl::expected`-style; never silent.
- Optional lookups → `std::optional`.
- Logging on every failure (currently silent in renderers); use `BOOST_LOG_TRIVIAL`
  consistently — drop the mixed `std::cout` lines (e.g.
  `renderer/FPGAFTDIRenderer/FPGARendererFTDI.cpp:38`).

**Quick wins safe to bundle into another PR:**
- `renderer/FPGAFTDIRenderer/FPGARendererFTDI.cpp:43-48` and the matching
  `renderer/FPGASPIRenderer/FPGARendererRPISPI.cpp` site — silent `try_lock` failures
  should at least log at `BOOST_LOG_TRIVIAL(warning)`.

---

### C3 — Naming consistency *(Partial — config files done, formatting commit pending)*

**Status:** 🔧 PARTIAL — Commit `12599a1` added config files. Formatting commit not yet done.

**Done:**
- `.clang-tidy` — enables `modernize-*`, `readability-identifier-naming`, `cppcoreguidelines-*`. Naming: `camelCase` members with optional trailing `_` for shadowed names.
- `.clang-format` — 4-space indent, K&R braces, `ColumnLimit: 150`, pointer/reference left-aligned.
- `.git-blame-ignore-revs` — template with placeholder for formatting commit SHA.

**Remaining:**
1. Install `clang-format` (`brew install clang-format`)
2. Run `clang-format -i` on all `.h` and `.cpp` files in `common/`, `server/`, `application/`, `renderer/`
3. Verify build and tests pass
4. Commit with message `style: apply clang-format to codebase`
5. Add commit SHA to `.git-blame-ignore-revs`
6. (Optional) Add CI step that runs `clang-tidy --warnings-as-errors=…` on changed files

**Note:** `clang-format` and `clang-tidy` binaries were not installed on the dev machine during the session. Install before running.

---

### C4 — Consolidate config loading *(Not done — partial overlap with existing protobuf config)*

**Status:** Not done. The `matrixserver::ServerConfig` protobuf already plays the
"single Config type loaded once at startup and passed by `const &`" role for the
server side. The remaining hardcoded values in source either need protobuf schema
additions (out of scope per the original plan: "Don't touch the protobuf schema
during this refactor") or are sensible client-side defaults.

**Why deferred:** Most of the value is already realized via the existing
`ServerConfig`. The remaining hardcoded constants need protobuf schema changes to
become configurable — explicitly excluded from the original refactor scope.

**Task-specific info (from original plan):**

`matrixServerConfig.json` is currently parsed in scattered places (Server,
ServerSetup, MainMenu). Introduce a single `Config` POD-ish type loaded once at
startup and passed by `const &` everywhere. Removes hardcoded defaults like `1337`,
`127.0.0.1:2017`, `/usr/local/bin/MainMenu`, `parallel = 1` from source.

**Concrete remaining work:**
- `renderer/RGBMatrixRenderer/RGBMatrixRenderer.cpp:33` — `RGBmatrixOptions.parallel
  = 1; // TODO: add to global config`. Add `RgbMatrixOptions { int parallel; int
  chain_length; … }` to `common/protobuf/matrixserver.proto` under `ServerConfig`.
  `ServerSetup::createDefaultCubeConfig` populates per-HardwareType defaults.
  `RGBMatrixRenderer` ctor reads from the passed config.
- Audit FPGA renderers for similar hardcoded constants (chain length, brightness).
- Bump version to `0.5.0` for the additive proto change (back-compat).
- The client-side defaults (`DEFAULTSERVERURI`, `DEFAULTFPS`, etc.) in
  `application/MatrixApplication.h` are sensible as `inline constexpr` and should
  stay.

---

### C5 — Tests *(DONE)*

**Status:** ✅ DONE — Commit `12599a1`. Test files created and verified:
- `tests/tests-message-dispatcher.cpp` — 5 TEST_CASEs with `[dispatcher]` tag
- `tests/tests-connection-factory.cpp` — 6 TEST_CASEs with `[connection-factory]` tag
- `tests/tests-input-state.cpp` — 9 TEST_CASEs with `[input-state]` tag

**Also fixed:** Pre-existing protobuf duplicate descriptor crash in test binary. Root cause: `matrixserver.pb.cc` linked both directly (via `libcommon.a`) and transitively (via `libmatrixapplication.dylib`). Fix: removed redundant `common` from `target_link_libraries(testAll ...)` in `tests/CMakeLists.txt`.

All 115 tests pass (4442 assertions, 0 failures).

**Not added (from original spec):**
- `tests/tests-default-app-launcher.cpp` — listed as optional/harder in spec
- `tests/tests-server-bootstrap.cpp` — listed as optional/harder in spec

---

## Newly discovered items (not in the original plan)

### N1 — Extract `RendererRegistry` from `Server` *(DONE)*

**Status:** ✅ DONE — Commit `12599a1`.

New `server/RendererRegistry.{h,cpp}` with:
- `add(std::shared_ptr<IRenderer> r)` — does the `dynamic_pointer_cast`, populates both lists
- `renderAll()` — calls `render()` on each
- `setScreenData(int sid, const Color* data)` — fans out
- `setBrightness(int b)` — applies to each
- `broadcastMessage(std::shared_ptr<MatrixServerMessage> msg)` — biDir only
- `setMessageCallback(IBidirectionalRenderer::MsgCallback cb)` — biDir only
- `forEachRenderer(fn)` — per-renderer iteration (needed for `handleSetScreenFrame` ack loop)

`Server` holds `RendererRegistry registry_` member and delegates to it.

---

### N2 — Decisions baked into the refactor that are worth knowing

These are not new tasks, just gotchas to be aware of when picking up the next
refactor:

- **Per-renderer ack on `setScreenFrame`** (preserved from pre-refactor code):
  `Server::handleSetScreenFrame` sends one ack per renderer inside the renderer
  fan-out loop. With multiple renderers (e.g., the param-only `WebSocketSimulator`
  alongside a hardware renderer), the client receives multiple acks per frame. The
  Phase 1 condvar handshake in `MatrixApplication::internalLoop` waits for one
  notify, so extra acks are no-ops. Don't change this without a coordinated
  app-side audit — apps may rely on the current behaviour.

- **`appKill` self-join** in `MatrixApplication::stop()`: when the server sends
  `appKill`, `handleRequest` runs on the IO thread and calls `stop()`. `stop()`
  detects `ioThread->get_id() == std::this_thread::get_id()` and `detach()`s
  instead of `join()`-ing. This avoids deadlock; the alternative is a separate
  shutdown thread. Works correctly today.

- **`MessageDispatcher` silently drops unknown message types** at trace level —
  matches the pre-refactor silent-default behaviour. If protocol versioning is
  added in the future, this is the place to distinguish "expected-unknown" vs
  "suspicious-unknown".

---

### N3 — LC_RPATH fix in example apps' CMake (sits in the example-apps repo)

**Status:** Discovered during Phase 1 verification. Lives in the example-apps repo,
not matrixserver — listed here for completeness.

**The gotcha:** Example app binaries bake `LC_RPATH = /usr/local/lib` only. After
`make install` to `matrixserver/install/`, the *build* uses the refactored lib (API
verified at link time), but at *runtime* the dynamic loader falls back to
`/usr/local/lib/libmatrixapplication.dylib` — the previously installed system copy.
Documented in both READMEs and the parent CLAUDE.md.

**Concrete fix (example-apps repo):**
In each app's `CMakeLists.txt` (or top-level), set:
```cmake
set_target_properties(${TARGET} PROPERTIES
    BUILD_RPATH "${CMAKE_PREFIX_PATH}/lib"
    INSTALL_RPATH "/usr/local/lib;${CMAKE_PREFIX_PATH}/lib")
```
Or use `CMAKE_INSTALL_RPATH_USE_LINK_PATH = ON` to derive automatically.

---

## Build & verification setup (applies to every phase)

*(Copied unchanged from the original plan — every PR should follow this flow.)*

Example apps' CMake finds the matrixserver lib via `find_package(matrixapplication)`
against `CMAKE_PREFIX_PATH=matrixserver/install/`. So the canonical refactor
verification flow is:

```bash
# 1. Build + install matrixserver into the local install tree
cd matrixserver && rm -rf build install && mkdir -p build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$(pwd)/../install
cmake --build . --target install -j

# 2. Run unit tests (these run against the in-tree build, no rpath issue)
make testAll && ./tests/testAll

# 3. Clean-build example apps so they pick up freshly installed headers + lib
cd ../.. && rm -rf build && mkdir -p build && cd build
cmake .. && cmake --build . -j
```

**LC_RPATH gotcha — read before running any example-app binary:**

The example apps' CMake bakes `LC_RPATH = /usr/local/lib` into each binary. After
`make install` to `matrixserver/install/`, the *build* uses the refactored lib
(verifies API), but at *runtime* the dynamic loader still falls back to
`/usr/local/lib/libmatrixapplication.dylib` — the previously-installed system copy.
To actually run an example app against the freshly built lib, point the loader at
the install tree:

```bash
# macOS
export DYLD_LIBRARY_PATH=$(pwd)/../matrixserver/install/lib
# Linux
export LD_LIBRARY_PATH=$(pwd)/../matrixserver/install/lib

./bin/Snake
```

Alternatives:
- `sudo make install` matrixserver to `/usr/local` so the rpath happens to match
  (intrusive — replaces the system copy).
- `install_name_tool -add_rpath $(pwd)/../matrixserver/install/lib bin/<App>`
  post-build (per-binary patch, macOS only).

---

## Risks / things to watch

*(Copied from the original plan — still relevant.)*

- **Public API of `MatrixApplication`** is consumed by every example app in
  `LED_Cube-Example_Applications/`. Any signature change ripples; decompose *behind*
  the existing API where possible.
- **Protobuf schema** (`common/protobuf/matrixserver.proto`) is the wire format
  between server and apps — do not touch unless explicitly scoped (C4 / F8 would
  scope it).
- **Hardware backends are difficult to test in CI**; rely on the simulator for
  automated coverage and document a manual smoke-test checklist (FPGA + RGB matrix)
  for hardware-touching changes.
- **One PR per layer**, not one mega-PR. Reviewers (and bisect) will thank you.
  Each PR should be independently shippable and leave the tree green.
- **Stale clangd diagnostics** reliably appear after `rm -rf build` (the
  `compile_commands.json` symlink breaks). Always verify with an actual `cmake
  --build` before reacting to clangd errors.
- **Pre-existing diagnostics that are NOT bugs:** `renderer/FPGASPIRenderer/
  FPGARendererRPISPI.cpp` references `linux/spi/spidev.h`, which doesn't exist on
  macOS — that file is only compiled in the RPi backend path.

---

## Exit criteria still outstanding

From the original plan, these criteria are not yet met:

- [x] Zero raw `new` / `delete` for resources (smart pointers or RAII members).
- [x] Every thread (in matrixserver core) has a destructor that joins it. *(P0
  finishes this for Mpu6050, ADS1000, MatrixApplicationStandalone,
  WebSocketSimulatorRenderer.)*
- [x] `MatrixApplication.cpp` and `Server.cpp` each under ~250 LOC. *(Phase 3a,
  4a, and S2 LoopRunner extraction brought both close.)*
- [ ] **One server executable, runtime backend selection.** *(S4 partial — needs
  CI/Docker/Debian coordination.)*
- [x] **Test coverage adds at least: dispatcher tests, connection factory tests,
  app-lifecycle tests.** *(C5 DONE — 115 tests, 4442 assertions.)*
- [x] New-developer setup: `cmake .. && make`, `./server_simulator`, `./MainMenu`,
  see a cube — without reading more than the top-level `README.md` and one
  architecture diagram.

**One unmet criterion remains:** S4 (single binary with runtime backend selection).
