# Roadmap — Berlin Vocal AI

Phase boundaries follow the project brief. Nothing below is
implemented in Phase 1; this document only records direction.

## Phase 1 (this repository) — DONE criteria

- [x] CMake + JUCE 8 project, VST3 target
- [x] Full vocal chain with safe defaults and bypass controls
- [x] Stable parameter IDs, automation, state save/restore
- [x] Clean GUI, real parameters only
- [x] Headless test suite (64 checks) — all passing
- [x] Documentation (this folder)
- [ ] **Windows/MSVC build on the user's machine**
- [ ] **Mixcraft load + smoke test on the user's machine**

## Phase 2 (planned, not started)

Local companion/control layer + optional GLM integration:

1. **Structured action protocol.** A validated, bounded JSON-ish plan
   ("set compThreshold to −24", "save preset 'Podcast'") produced from
   a user request. The DSP engine stays AI-provider-agnostic — GLM is
   behind an interface that can be swapped.
2. **Preview / approve / revert.** Every AI-suggested change is shown
   as a diff, applied only on explicit approval, and reversible in one
   click (snapshot of the previous parameter state).
3. **Preset management.** Named presets stored locally, independent of
   host state.
4. **DSP refinements** (from Phase 1 learnings):
   - compressor: RMS/peak blend, soft knee option, sidechain HPF,
   - de-esser: lookahead or split-band envelope refinement,
   - true-peak metering.
5. **Parameter version hints** when a supported JUCE upgrade exposes
   the setter; until then, migration via new parameter IDs.

## Explicitly out of scope (any phase, per product decision)

Cloud processing, telemetry, accounts, auto-updates, voice cloning,
stem separation, automatic mastering decisions.

## Engineering conventions going forward

- Every new DSP module ships with deterministic tests in the same PR.
- Parameter IDs: add-only; never rename.
- Keep `docs/DSP_NOTES.md` updated with any new "implementation trap"
  discovered against the pinned JUCE version.
