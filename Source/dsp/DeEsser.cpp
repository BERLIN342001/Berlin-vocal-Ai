#include "DeEsser.h"

namespace bva
{

void DeEsser::prepare (double sampleRateToUse, int numChannels)
{
    jassert (sampleRateToUse > 0.0);
    jassert (numChannels >= 0 && numChannels <= maxChannels);

    sampleRate = sampleRateToUse;
    envelope.fill (0.0f);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) juce::jmax (1, numChannels), 1 };

    // IMPORTANT: the default LinkwitzRileyFilter type is lowpass for BOTH
    // objects - the high-pass side MUST be set explicitly (verified against
    // the pinned JUCE 8.0.15 source).
    for (auto& f : lowPass)
    {
        f.setType (juce::dsp::LinkwitzRileyFilter<float>::Type::lowpass);
        f.prepare (spec);
    }
    for (auto& f : highPass)
    {
        f.setType (juce::dsp::LinkwitzRileyFilter<float>::Type::highpass);
        f.prepare (spec);
    }

    for (auto& g : gainSmooth)
    {
        g.reset (sampleRate, 0.003); // 3 ms gain smoothing
        g.setCurrentAndTargetValue (1.0f);
    }

    updateCrossover();

    // Recompute ballistics for the new sample rate (~1 ms attack, ~40 ms release).
    attackCoef  = 1.0f - std::exp (-1.0f / (0.001f * (float) sampleRate));
    releaseCoef = 1.0f - std::exp (-1.0f / (0.040f * (float) sampleRate));
}

void DeEsser::reset()
{
    for (auto& f : lowPass)  f.reset();
    for (auto& f : highPass) f.reset();
    envelope.fill (0.0f);
    for (auto& g : gainSmooth)
        g.setCurrentAndTargetValue (1.0f);
}

void DeEsser::setParameters (float crossoverHzNew, float thresholdDbNew, float maxAmountDbNew)
{
    crossoverHz = juce::jlimit (1000.0f, (float) (sampleRate * 0.45), crossoverHzNew);
    thresholdDb = juce::jlimit (-60.0f, 0.0f, thresholdDbNew);
    amountDb    = juce::jlimit (0.0f, 24.0f, maxAmountDbNew);

    updateCrossover();
}

void DeEsser::setCrossoverHz (float crossoverHzNew)
{
    crossoverHz = juce::jlimit (1000.0f, (float) (sampleRate * 0.45), crossoverHzNew);
    updateCrossover();
}

void DeEsser::updateCrossover()
{
    const float safeFreq = juce::jlimit (1000.0f, (float) (sampleRate * 0.45), crossoverHz);
    for (auto& f : lowPass)  f.setCutoffFrequency (safeFreq);
    for (auto& f : highPass) f.setCutoffFrequency (safeFreq);
}

} // namespace bva
