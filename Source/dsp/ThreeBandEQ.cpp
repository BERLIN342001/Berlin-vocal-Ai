#include "ThreeBandEQ.h"

namespace bva
{

void ThreeBandEQ::prepare (double sampleRate, int numChannels)
{
    jassert (sampleRate > 0.0);
    jassert (numChannels >= 0 && numChannels <= maxChannels);

    currentSampleRate = sampleRate;

    for (auto& band : filters)
        for (auto& f : band)
            f.prepare ({ sampleRate, 1, 1 });

    // Apply the current coefficients to EVERY channel (a fresh filter's
    // default coefficients are NOT neutral - they are a comb).
    for (int ch = 0; ch < maxChannels; ++ch)
    {
        setBand (filters[ch][0], 0, low.freqHz,  low.gainDb,  low.q);
        setBand (filters[ch][1], 1, mid.freqHz,  mid.gainDb,  mid.q);
        setBand (filters[ch][2], 2, high.freqHz, high.gainDb, high.q);
    }
}

void ThreeBandEQ::reset()
{
    for (auto& band : filters)
        for (auto& f : band)
            f.reset();
}

void ThreeBandEQ::updateCoefficients (const EQBandParams& lowP,
                                      const EQBandParams& midP,
                                      const EQBandParams& highP)
{
    low = lowP; mid = midP; high = highP;

    // Defensive clamping (params are already range-bound by APVTS, but the
    // DSP layer must never trust that).
    auto safeFreq = [sr = currentSampleRate] (float f)
    {
        return juce::jlimit (20.0f, (float) (sr * 0.45), f);
    };
    auto safeQ = [] (float q) { return juce::jlimit (0.05f, 20.0f, q); };
    auto safeGain = [] (float g) { return juce::jlimit (-24.0f, 24.0f, g); };

    setBandAllChannels (0, safeFreq (low.freqHz),  safeGain (low.gainDb),  safeQ (low.q));
    setBandAllChannels (1, safeFreq (mid.freqHz),  safeGain (mid.gainDb),  safeQ (mid.q));
    setBandAllChannels (2, safeFreq (high.freqHz), safeGain (high.gainDb), safeQ (high.q));
}

void ThreeBandEQ::setBandAllChannels (int kind, float freqHz, float gainDb, float q)
{
    for (int ch = 0; ch < maxChannels; ++ch)
        setBand (filters[ch][kind], kind, freqHz, gainDb, q);
}

void ThreeBandEQ::setBand (juce::dsp::IIR::Filter<float>& filter, int kind,
                           float freqHz, float gainDb, float q)
{
    using Coeffs = juce::dsp::IIR::Coefficients<float>;
    Coeffs::Ptr newCoeffs;

    switch (kind)
    {
        case 0:
            newCoeffs = Coeffs::makeLowShelf (currentSampleRate, freqHz, q,
                                              juce::Decibels::decibelsToGain (gainDb));
            break;
        case 1:
            newCoeffs = Coeffs::makePeakFilter (currentSampleRate, freqHz, q,
                                                juce::Decibels::decibelsToGain (gainDb));
            break;
        case 2:
        default:
            newCoeffs = Coeffs::makeHighShelf (currentSampleRate, freqHz, q,
                                               juce::Decibels::decibelsToGain (gainDb));
            break;
    }

    // Atomic pointer swap: click-safe parameter updates.
    *filter.coefficients = *newCoeffs;
}

} // namespace bva
