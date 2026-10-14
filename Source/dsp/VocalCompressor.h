#pragma once
/*
    VocalCompressor.h
    -----------------
    Feed-forward peak compressor, one instance per channel.

    Design (documented in docs/DSP_NOTES.md):
    - Peak envelope follower with program-dependent smoothing:
      attack/release from parameters.
    - Standard hard-knee gain computer:
          over    = envDb - thresholdDb
          gainDb  = -over * (1 - 1/ratio)     (only when over > 0)
    - The computed gain is smoothed (5 ms) so threshold/ratio edits do
      not click, and the envelope itself drives attack/release.
    - Makeup gain is applied AFTER compression via a ramped gain stage
      owned by VocalProcessor (separate parameter).

    Deliberately simple and predictable for Phase 1. Known limitations
    are listed in docs/DSP_NOTES.md (no soft knee, no lookahead, no
    sidechain HPF, peak not RMS).
*/

#include <juce_dsp/juce_dsp.h>

namespace bva
{

class VocalCompressor
{
public:
    static constexpr int maxChannels = 2;

    void prepare (double sampleRate, int numChannels);
    void reset();

    // Called once per block from the audio thread.
    void setParameters (float thresholdDb, float ratioTo1,
                        float attackMs, float releaseMs);

    // Real-time safe per-sample processing.
    inline float processSample (int channel, float sample)
    {
        const float x = sample;

        // Peak envelope follower (per channel).
        const float xAbs = std::abs (x);
        float& env = envelope[channel];
        if (xAbs > env)
            env += attackCoef  * (xAbs - env);
        else
            env += releaseCoef * (xAbs - env);

        // Gain computer (hard knee).
        const float envDb    = juce::Decibels::gainToDecibels (env, -120.0f);
        const float overDb   = envDb - thresholdDb;
        const float slope    = 1.0f - (1.0f / juce::jmax (1.0f, ratio));
        const float targetDb = overDb > 0.0f ? -(overDb * slope) : 0.0f;

        gainSmooth[channel].setTargetValue (juce::Decibels::decibelsToGain (targetDb, -120.0f));
        return x * gainSmooth[channel].getNextValue();
    }

private:
    std::array<float, maxChannels> envelope {};
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, maxChannels> gainSmooth;

    double sampleRate = 44100.0;
    float  thresholdDb = -18.0f;
    float  ratio       = 2.0f;
    float  attackMsParam  = 10.0f;   // stored so prepare() can recompute
    float  releaseMsParam = 100.0f;  // ballistics after a rate change
    float  attackCoef  = 0.1f;
    float  releaseCoef = 0.001f;
};

} // namespace bva
