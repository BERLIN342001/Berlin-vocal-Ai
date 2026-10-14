# Architecture — Berlin Vocal AI, Phase 1

## Component map

```
                ┌──────────────────────────────────────────────┐
   DAW host     │ BerlinVocalAIAudioProcessor (AudioProcessor) │
 ── audio ────► │   ┌──────────────────────────────────────┐   │
 ── params ───► │   │ APVTS (25 stable-ID parameters)      │   │
 ── state ─────► │   └──────────────┬───────────────────────┘   │
                │                  │ snapshotValues() per block│
                │   ┌──────────────▼───────────────────────┐   │
                │   │ bva::VocalProcessor (audio thread)   │   │
                │   │   gains / HPF / EQ / comp / de-esser │   │
                │   │   dry-wet / master bypass            │   │
                │   │   LevelMeterSource (atomics out)     │   │
                │   └──────────────────────────────────────┘   │
                │            ▲ atomic reads only               │
                │   ┌──────────────────────────────────────┐   │
                │   │ BerlinVocalAIAudioProcessorEditor    │   │
                │   │   sliders/toggles ── APVTS attachments│  │
                │   │   meters (30 Hz timer)               │   │
                │   └──────────────────────────────────────┘   │
                └──────────────────────────────────────────────┘
```

## Ownership and threading

| Object | Thread(s) | Notes |
|---|---|---|
| `APVTS` parameter atomics | GUI writes, audio reads | JUCE-managed; raw `std::atomic<float>*` reads are real-time safe |
| `VocalProcessor` | **Audio thread only** | created/owned by the processor; prepared in `prepareToPlay` |
| Editor + attachments | Message thread | never touch `VocalProcessor` directly |
| `LevelMeterSource` atomics | audio writes, GUI reads | relaxed ordering; GUI decays visuals on a timer |

**The parameter → DSP path is one-directional:** `processBlock()`
snapshots all 25 atomics into a POD (`ParameterValues`) and pushes it
into `VocalProcessor::setParameterValues()` once per block. The DSP
never calls back into the parameter system.

## Real-time safety contract (`VocalProcessor`)

- **No heap allocation** in `process()` — all state is value-owned
  members prepared in `prepareToPlay()`.
- **No locks, no file/network I/O, no logging** in the audio path.
- `juce::ScopedNoDenormals` per block; a final defensive sweep replaces
  any non-finite sample with 0 (belt-and-braces; tests assert it is
  never needed under valid input).
- Parameter smoothing:
  - gain stages: 20 ms ramps (`SmoothedValue`, per-sample),
  - module toggles (HPF/comp/de-esser): 10 ms crossfade between the
    processed and dry paths — modules keep running so their state never
    jumps,
  - dry/wet: 30 ms equal-gain crossfade,
  - master bypass: 50 ms crossfade back to the untouched input.
- The master bypass output is **bit-exact** to the input once the ramp
  settles (verified by test).

## Bus layout

Supported and tested: **mono→mono** and **stereo→stereo** (identical
in/out). Everything else is rejected by `isBusesLayoutSupported`.
The DSP engine always prepares for up to 2 channels and processes
`min(buffer channels, 2)`.

## Latency

The chain introduces **zero latency** (no lookahead anywhere);
`getLatencySamples()` stays 0 and is asserted by a test.

## State

`getStateInformation` serialises the APVTS state as XML inside the
host's chunk; `setStateInformation` validates the root tag and calls
`replaceState` (the supported path — attachments and automation lanes
resync automatically). Corrupt blobs are ignored without crashing
(verified by test).

Parameter IDs are stable strings (see `Source/Parameters.h`). Do NOT
rename them after a release; the Phase 1 JUCE version exposes no
parameter version-hint setter, so future changes must add NEW IDs and
migrate on load (noted in `PluginProcessor.cpp`).

## GUI

One fixed-size editor (700×900), grouped by signal flow. All knobs are
`RotaryHorizontalVerticalDrag` with value units (dB/Hz/ms/%), all bound
through `AudioProcessorValueTreeState::SliderAttachment` /
`ButtonAttachment`, so GUI ↔ host automation ↔ restored state can never
desynchronise. Meters poll the atomic sources at 30 Hz and decay
visually on the message thread.
