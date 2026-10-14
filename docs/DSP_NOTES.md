# DSP notes — Berlin Vocal AI, Phase 1

Engineering rationale and known limitations of every module. Sample
rates mentioned as examples assume 44.1 kHz.

## Signal chain

```
x ─► inputGain(±24 dB, 20 ms ramp)
  ─► [HPF 24 dB/oct]            (toggle: 10 ms crossfade)
  ─► 3-band EQ                  (always engaged; 0 dB ≈ transparent)
  ─► [Compressor + makeup]      (toggle: 10 ms crossfade)
  ─► [De-esser]                 (toggle: 10 ms crossfade)
  ─► outputGain(±24 dB, 20 ms ramp)
  ─► dry/wet (30 ms equal-gain crossfade)
  ─► master bypass (50 ms crossfade to untouched input)
```

All smoothing advances **once per sample** in a single loop; both
channels share the same smoothing values per sample (stereo-linked).

## High-pass filter

Two cascaded 2nd-order Butterworth high-pass biquads at the same
cutoff = 24 dB/oct total. Verified: 40 Hz tone @ cutoff 80 Hz
attenuates by **−24.6 dB** (theory −24.6 dB), 1 kHz passes within
−0.01 dB.

Coefficient updates are atomic pointer swaps (`*f.coefficients = *c`),
so live cutoff moves are click-safe.

## Three-band EQ

- Low: low shelf (40–500 Hz), Mid: peaking (200 Hz–5 kHz),
  High: high shelf (2–16 kHz). Gains ±15 dB, Q 0.1–10.
- **There is no EQ enable toggle**: with all gains at 0 dB the bands
  measure within ±0.01 dB of transparent — the neutral state IS the
  bypass. Keeping the filters engaged avoids transition clicks and
  keeps the code honest.
- Verified: +12 dB mid boost @ 1 kHz measures +12.00 dB; +12 dB low
  shelf @ 60 Hz measures +11.12 dB relative to the flat chain
  (a shelf one octave below its corner gives slightly under full
  boost at Q 0.707 — expected shelf behaviour).

⚠ **Implementation trap (found by tests):** a default-constructed
`juce::dsp::IIR::Filter` is NOT neutral — its default coefficients are
`(1, 0, 1, 0)`-family comb behaviour, and a default
`juce::dsp::IIR::Coefficients` is all zeros. Every channel's filters
must receive real coefficients in `prepare()` AND on every parameter
update. `ThreeBandEQ` sets coefficients for **all channels**
(`setBandAllChannels`) for exactly this reason.

## Compressor (`VocalCompressor`)

Custom feed-forward design (not `juce::dsp::Compressor`) for full
control and testability:

- **Detection**: per-channel peak envelope follower, one-pole
  ballistics from the attack/release parameters (10 ms/100 ms defaults).
- **Gain computer**: hard knee, `gainDb = −(env−thresh)·(1 − 1/ratio)`
  for env above threshold.
- **Gain smoothing**: 5 ms linear ramp on the computed gain so
  threshold/ratio edits don't click.
- **Makeup**: separate ramped gain applied to the compressed path
  only (before the module's own crossfade), 0–24 dB.
- Verified: 2:1 @ −18 dB on a −3 dBFS tone reduces ≈ 7 dB (theory
  ≈ 7.4 dB with peak-vs-RMS envelope differences); transparent below
  threshold (−0.15 dB).

Limitations: peak (not RMS) detection, hard knee only, no lookahead,
no sidechain high-pass — bass-heavy material may pump. All candidates
for Phase 2 refinement.

## De-esser (`DeEsser`) — simple split-band prototype

- **Split**: Linkwitz-Riley 4th-order pair at the crossover
  (3–12 kHz, default 6.5 kHz). Verified against the pinned JUCE source:
  LP+HP sums to unity magnitude at all test frequencies.
- **Detection**: peak envelope of the high band only, ~1 ms attack /
  ~40 ms release.
- **Gain computer**: fixed 3:1 slope, clamped to the user's maximum
  amount (0–24 dB, default 6 dB), 3 ms smoothed gain on the high band.
- **Recombine**: `low + high·gain`.

Verified: −3.7 dB reduction on sibilance-like noise; low band
untouched (−0.01 dB at 500 Hz even at −60 dB threshold).

⚠ **Implementation trap (found by tests):** `LinkwitzRileyFilter`'s
default type is **lowpass for every instance** — the high-pass side
MUST call `setType(Type::highpass)` explicitly. Without it, the
"high band" is a second low-pass copy of the full signal, the envelope
sees the whole signal, and the de-esser *boosts* the signal by ~+3.5 dB
(y = LP + LP·0.5). This exact bug was caught by the test suite.

**Honest limitations (this is NOT a studio-grade de-esser):**
- Peak detection, no lookahead: very fast sibilant transients can pass
  before the gain engages.
- Fixed 3:1 slope; no frequency-dependent release.
- LR4 crossover introduces a small phase shift around the crossover
  point; with moderate amounts this is inaudible in most material but
  it is not phase-linear.

## Level meters

Per-block RMS over all channels, published through
`std::atomic<float>` with a decay factor (peak-hold feel). The GUI
decays further on a 30 Hz timer. Mono-summed RMS — not true peak.

## Non-finite safety

- Inputs are range-clamped at the DSP boundary (`jlimit`) even though
  APVTS already clamps.
- A final sweep zeroes any non-finite output sample (never triggered
  in the 64-check suite, including extreme ±24 dB gain tests).
- Determinism test: two identically-configured processors produce
  **bit-exact** output — no hidden randomness or time dependence.
