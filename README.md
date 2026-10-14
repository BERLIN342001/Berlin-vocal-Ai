# Berlin Vocal AI — Phase 1

A native **VST3 vocal-processing plugin** for Windows (and Linux, for
development), built with **JUCE 8 / C++17 / CMake**.

> **Status: Phase 1 — Foundation.** This is a reliable audio-processing
> foundation, NOT an AI product yet. No GLM, no cloud, no API keys, no
> internet connection required or used.

![Phase](https://img.shields.io/badge/phase-1%20%E2%80%94%20foundation-blue)

---

## What Phase 1 contains

A restrained, reliable vocal chain with sensible defaults:

```
input ─> Input Gain ─> [High-Pass 24 dB/oct] ─> 3-Band EQ ─> [Compressor]
      ─> [De-esser] ─> Output Gain ─> Dry/Wet ─> (Master Bypass) ─> out
```

- **High-pass filter**: 24 dB/oct (two cascaded biquads), 20–250 Hz,
  on by default at 80 Hz.
- **Three-band parametric EQ**: low shelf / peaking mid / high shelf,
  each with frequency, gain and Q.
- **Compressor**: feed-forward peak detector, hard knee, threshold /
  ratio / attack / release + separate makeup gain.
- **De-esser**: simple split-band prototype (Linkwitz-Riley 4th-order
  crossover, peak detection, fixed 3:1 slope). Honest about its limits —
  see `docs/DSP_NOTES.md`.
- **Click-free everywhere**: every gain, toggle and mix control is
  sample-accurate smoothed (20 ms gains, 10 ms module toggles, 30 ms
  dry/wet, 50 ms master bypass).
- **25 exposed parameters** with stable IDs, full DAW automation,
  and complete state save/restore through the host.
- **GUI**: signal-flow layout, labelled controls with units and
  tooltips, input/output level meters, master bypass with visual
  indication. Every control is bound to a real parameter — no fake
  buttons, no placeholders.
- **Headless test suite**: 64 deterministic checks covering every
  module, state, layouts, NaN safety, determinism and re-initialisation
  (`Tests/`). All passing at commit time.

## Quick start (Windows)

1. Install **Visual Studio 2022** (Desktop development with C++) and
   **CMake 3.22+**.
2. From a developer prompt:

   ```bat
   git clone <this-repo> BerlinVocalAI
   cd BerlinVocalAI
   cmake -S . -B build -G "Visual Studio 17 2022" -A x64
   cmake --build build --config Release
   ```

   The first configure downloads JUCE 8.0.15 automatically (pinned tag).
3. The VST3 appears at
   `build\BerlinVocalAI_artefacts\Release\VST3\Berlin Vocal AI.vst3`.
4. Copy it to `C:\Program Files\Common Files\VST3\` and rescan in your
   DAW (Mixcraft: Options → Plug-in manager → rescan).

Full details and troubleshooting: `docs/BUILD_WINDOWS.md`.

## Repository layout

```
BerlinVocalAI/
├── CMakeLists.txt          JUCE 8.0.15 via pinned FetchContent
├── Source/
│   ├── PluginProcessor.*   host-facing processor, state, buses
│   ├── PluginEditor.*      GUI (all controls attached to parameters)
│   ├── Parameters.*        central parameter layout & defaults
│   └── dsp/
│       ├── VocalProcessor.*    chain owner (audio thread only)
│       ├── ThreeBandEQ.*       shelf/peak/shelf biquads
│       ├── VocalCompressor.*   feed-forward peak compressor
│       ├── DeEsser.*           split-band prototype
│       └── LevelMeter.h        audio→GUI meter atomics
├── Tests/                  headless deterministic test suite
└── docs/                   build / architecture / DSP / roadmap
```

## Parameter defaults (engineering starting points, not presets)

| Parameter | Default | Range |
|---|---|---|
| Input Gain | 0 dB | −24…+24 dB |
| HPF | on, 80 Hz | 20…250 Hz |
| EQ Low / Mid / High | 0 dB @ 120 Hz / 1 kHz / 8 kHz | ±15 dB |
| Compressor | off (−18 dB, 2:1, 10 ms, 100 ms, 0 dB) | — |
| De-esser | off (6.5 kHz, −30 dB, 6 dB) | — |
| Output Gain | 0 dB | −24…+24 dB |
| Dry/Wet | 100 % wet | 0…100 % |
| Master Bypass | off | — |

## Verified vs. not verified (honesty table)

| Item | Status |
|---|---|
| CMake configure + full build (Linux, GCC 14, JUCE 8.0.15) | **Implemented & verified** |
| Headless test suite (64 checks) | **Implemented & verified, all passing** |
| DSP behaviour (HPF slope, EQ gains, comp reduction, de-esser band) | **Verified** against theory on Linux |
| VST3 binary loads in a DAW | **Not verified here** — requires Windows + Mixcraft (see docs/BUILD_WINDOWS.md §5) |
| Windows/MSVC compilation | **Not verified here** — no MSVC in this environment; code uses standard JUCE/C++17 APIs |
| GUI rendering on Windows | Implemented, compile-verified via Linux build + headless link; visual check pending on your machine |

## Known limitations (Phase 1)

- The de-esser is a **prototype**: peak detection, no lookahead, fixed
  3:1 slope, LR4 crossover phase shift around the corner frequency.
- The compressor is hard-knee peak, no sidechain filter, no auto-makeup.
- Meters show a merged (mono-summed) RMS with peak-hold decay, not
  true peak / LUFS.
- No preset management beyond the host's own state mechanism.

See `docs/DSP_NOTES.md` for the full engineering discussion and
`docs/ROADMAP.md` for the planned Phase 2 (companion layer + GLM).

## License

Proprietary, all rights reserved for now. **JUCE is dual-licensed
(GPLv3 / commercial)** — a licensing decision must be made before any
distribution of binaries. See `LICENSE`.
