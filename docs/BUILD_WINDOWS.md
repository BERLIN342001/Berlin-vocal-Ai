# Building Berlin Vocal AI on Windows 10/11 x64

Tested toolchain expectation for this project (Phase 1):

| Component | Requirement |
|---|---|
| OS | Windows 10 or 11, x64 |
| Compiler | MSVC via **Visual Studio 2022** (or VS 2022 Build Tools), C++17 |
| CMake | 3.22 or newer |
| JUCE | 8.0.15 — fetched automatically at configure time (pinned tag, shallow clone) |
| Network | Required ONCE for the first configure (JUCE download ≈ 60–100 MB) |

---

## 1. Install the prerequisites

1. **Visual Studio 2022** — Community edition is fine. In the installer,
   select the *Desktop development with C++* workload (this provides
   MSVC, the Windows SDK, and `cmake`/`ninja` integration).
2. Verify from a *Developer Command Prompt for VS 2022*:

   ```bat
   cl
   cmake --version
   git --version
   ```

   If `cl` is not found, you are not in a developer prompt — always
   build from "x64 Native Tools Command Prompt for VS 2022" or use the
   Visual Studio GUI.

## 2. Get the source

```bat
git clone https://github.com/BERLIN342001/Berlin-vocal-Ai.git BerlinVocalAI
cd BerlinVocalAI
```

## 3. Configure and build

### Option A — Visual Studio GUI (recommended the first time)

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
```

Then open `build\BerlinVocalAI.sln`, select the **Release** config and
build the `BerlinVocalAI` target.

### Option B — Command line

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

Notes:
- The **first configure** clones JUCE 8.0.15 into `build\_deps\` — this
  needs internet once. Later re-configures are offline.
- Build outputs live in `build\` (outside the source tree on purpose).
- `JUCE_PATH` override: if you prefer a local JUCE copy, download
  JUCE 8.0.15, extract it, and add `-DJUCE_PATH=C:/path/to/JUCE` —
  the CMake script honours a pre-provided JUCE only if you add it
  before the FetchContent call (currently it always uses FetchContent;
  edit `CMakeLists.txt` if you need to vendor JUCE).

## 4. Run the headless test suite (optional but recommended)

```bat
cmake --build build --config Release --target bva_tests
build\bva_tests_artefacts\Release\bva_tests.exe
```

Expected output ends with `ALL TESTS PASSED` and exit code 0.

## 5. Install the plugin into your DAW (Mixcraft)

1. Locate the built plugin:
   `build\BerlinVocalAI_artefacts\Release\VST3\Berlin Vocal AI.vst3`
   (this is a folder — copy it **whole**).
2. Copy it to the system VST3 folder:
   `C:\Program Files\Common Files\VST3\`
   (Admin rights required. Alternative: configure
   `-DCOPY_PLUGIN_AFTER_BUILD=ON` in `juce_add_plugin` to auto-copy.)
3. In Mixcraft: **File → Preferences → Plug-ins → Rescan** (or delete
   Mixcraft's plugin cache and restart).
4. Insert "Berlin Vocal AI" on an audio track and confirm:
   - the GUI opens with the Phase 1 label,
   - moving controls audibly changes the signal,
   - automation of, e.g., Output Gain works from the DAW lane,
   - saving/reloading the project restores all knob positions.

## 6. Troubleshooting

| Symptom | Fix |
|---|---|
| `Could not find JUCE` / download fails | Check internet/proxy; or delete `build\_deps` and reconfigure |
| `CMake<3.22` error | Update CMake (VS 2022 ships one; use the developer prompt) |
| Plugin does not appear in Mixcraft | Verify the `.vst3` folder is in `Common Files\VST3`; rescan with "always find new plugins" enabled |
| Plugin appears but crashes | Run the test suite (§4) and report the failing test + Mixcraft version |
| `error C1083` on JUCE headers | Stale build dir — delete `build\` and reconfigure |

## 7. What was verified on Linux before this build

The exact same source tree was compiled and its 64-check test suite was
run to completion on Linux/GCC with JUCE 8.0.15 (a Linux VST3 binary
was produced). That validates the CMake setup, the DSP behaviour and
the headless logic — but **only a Windows build + Mixcraft load test
(§5) completes Phase 1's Definition of Done** on the target platform.
