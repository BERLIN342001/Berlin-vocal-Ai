#include "VocalCompressor.h"

namespace bva
{

void VocalCompressor::prepare (double sampleRateToUse, int numChannels)
{
    juce::ignoreUnused (numChannels);
    jassert (sampleRateToUse > 0.0);

    sampleRate = sampleRateToUse;
    envelope.fill (0.0f);

    for (auto& g : gainSmooth)
    {
        g.reset (sampleRate, 0.005); // 5 ms gain smoothing
        g.setCurrentAndTargetValue (1.0f);
    }

    // Recompute ballistics coefficients for the new sample rate using
    // the currently stored parameters.
    setParameters (thresholdDb, ratio, attackMsParam, releaseMsParam);
}

void VocalCompressor::reset()
{
    envelope.fill (0.0f);
    for (auto& g : gainSmooth)
        g.setCurrentAndTargetValue (1.0f);
}

void VocalCompressor::setParameters (float thresholdDbNew, float ratioTo1,
                                     float attackMs, float releaseMs)
{
    thresholdDb   = juce::jlimit (-60.0f, 0.0f, thresholdDbNew);
    ratio         = juce::jlimit (1.0f, 20.0f, ratioTo1);
    attackMsParam  = attackMs;
    releaseMsParam = releaseMs;

    const float attackSec  = juce::jlimit (0.0005f, 0.5f, juce::jmax (0.5f, attackMsParam)  * 0.001f);
    const float releaseSec = juce::jlimit (0.005f,  2.0f, juce::jmax (5.0f, releaseMsParam) * 0.001f);

    // One-pole coefficients (time constant to reach ~63% of a step).
    attackCoef  = 1.0f - std::exp (-1.0f / (attackSec  * (float) sampleRate));
    releaseCoef = 1.0f - std::exp (-1.0f / (releaseSec * (float) sampleRate));
}

} // namespace bva
