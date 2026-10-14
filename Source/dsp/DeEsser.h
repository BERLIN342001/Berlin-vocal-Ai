#pragma once
/*
    DeEsser.h
    --------
    SIMPLE split-band de-esser prototype (Phase 1).

    Signal is split at a crossover frequency (default 6.5 kHz, adjustable
    3-12 kHz) with a 4th-order Linkwitz-Riley pair. The high band drives a
    peak envelope detector; when the envelope exceeds the threshold, the
    high band is attenuated up to the configured amount with a fixed 3:1
    slope, then recombined with the untouched low band.

    THIS IS NOT A STUDIO-GRADE INTELLIGENT DE-ESSER. Known limitations
    (documented in docs/DSP_NOTES.md):
    - Peak detection, no lookahead: very fast sibilant transients can
      pass before the gain engages.
    - Fixed 3:1 slope; only threshold and max attenuation are exposed.
    - LR4 crossover sums with a small phase shift around the crossover
      point; with 100% mix and moderate amounts this is inaudible in
      most material, but it is not phase-linear.
*/

#include <juce_dsp/juce_dsp.h>

namespace bva
{

class DeEsser
{
public:
    static constexpr int maxChannels = 2;

    void prepare (double sampleRate, int numChannels);
    void reset();

    void setParameters (float crossoverHz, float thresholdDb, float maxAmountDb);
    void setCrossoverHz (float crossoverHz);

    inline float processSample (int channel, float sample)
    {
        const float low  = lowPass [channel].processSample (channel, sample);
        const float high = highPass[channel].processSample (channel, sample);

        const float hAbs = std::abs (high);
        float& env = envelope[channel];
        if (hAbs > env)
            env += attackCoef  * (hAbs - env);
        else
            env += releaseCoef * (hAbs - env);

        const float envDb  = juce::Decibels::gainToDecibels (env, -120.0f);
        const float overDb = envDb - thresholdDb;

        // Fixed 3:1 slope, clamped to the user's max amount.
        float targetDb = 0.0f;
        if (overDb > 0.0f && amountDb > 0.0f)
            targetDb = -juce::jmin (amountDb, overDb * (1.0f - 1.0f / 3.0f));

        gainSmooth[channel].setTargetValue (juce::Decibels::decibelsToGain (targetDb, -120.0f));

        return low + high * gainSmooth[channel].getNextValue();
    }

private:
    void updateCrossover();

    std::array<juce::dsp::LinkwitzRileyFilter<float>, maxChannels> lowPass;
    std::array<juce::dsp::LinkwitzRileyFilter<float>, maxChannels> highPass;

    std::array<float, maxChannels> envelope {};
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, maxChannels> gainSmooth;

    double sampleRate = 44100.0;
    float  crossoverHz = 6500.0f;
    float  thresholdDb = -30.0f;
    float  amountDb    = 6.0f;
    float  attackCoef  = 0.5f;   // ~1 ms at 44.1 kHz
    float  releaseCoef = 0.02f;  // ~40 ms
};

} // namespace bva
