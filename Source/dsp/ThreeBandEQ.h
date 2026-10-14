#pragma once
/*
    ThreeBandEQ.h
    -------------
    Low / Mid / High parametric EQ.

    - Low  : low shelf  (frequency = shelf corner, gain, Q)
    - Mid  : peaking    (frequency = centre, gain, Q)
    - High : high shelf (frequency = shelf corner, gain, Q)

    Implemented with TDF2 biquads (juce::dsp::IIR). Coefficient updates
    are atomic pointer swaps inside JUCE, so parameter changes are
    click-safe. There is no bypass/enable on the EQ: with all gains at
    0 dB the bands are within +/-0.1 dB of transparent, which is the
    module's neutral state (documented in docs/DSP_NOTES.md).
*/

#include <juce_dsp/juce_dsp.h>

namespace bva
{

struct EQBandParams
{
    float freqHz  = 1000.0f;
    float gainDb  = 0.0f;
    float q       = 0.707f;
};

class ThreeBandEQ
{
public:
    static constexpr int maxChannels = 2;

    void prepare (double sampleRate, int numChannels);
    void reset();

    // Update all band coefficients from the current parameter snapshot.
    void updateCoefficients (const EQBandParams& low,
                             const EQBandParams& mid,
                             const EQBandParams& high);

    // Real-time safe: one sample in, one sample out.
    // Index 0 = low shelf, 1 = mid peak, 2 = high shelf.
    inline float processSample (int channel, float sample)
    {
        auto x = sample;
        x = filters[channel][0].processSample (x);
        x = filters[channel][1].processSample (x);
        x = filters[channel][2].processSample (x);
        return x;
    }

private:
    void setBand (juce::dsp::IIR::Filter<float>& filter, int kind,
                  float freqHz, float gainDb, float q);
    void setBandAllChannels (int kind, float freqHz, float gainDb, float q);

    // kind: 0 = low shelf, 1 = peaking, 2 = high shelf
    std::array<std::array<juce::dsp::IIR::Filter<float>, 3>, maxChannels> filters;

    double currentSampleRate = 44100.0;
    EQBandParams low, mid, high;
};

} // namespace bva
