#pragma once
/*
    LevelMeter.h
    ------------
    Audio-side metering state. Computes a per-block RMS level with a
    peak-hold/decay ballistics on the AUDIO thread and publishes it
    through a plain std::atomic<float> that the GUI reads on a timer.

    This is the only cross-thread communication path for meters; no
    locks, no allocations (see docs/ARCHITECTURE.md).
*/

#include <juce_dsp/juce_dsp.h>

namespace bva
{

class LevelMeterSource
{
public:
    // Called from the audio thread once per block.
    void update (const float* const* channelData, int numChannels, int numSamples)
    {
        if (numChannels <= 0 || numSamples <= 0)
            return;

        double sumSquares = 0.0;
        long   count      = 0;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* data = channelData[ch];
            if (data == nullptr)
                continue;

            for (int i = 0; i < numSamples; ++i)
            {
                const float s = data[i];
                sumSquares += (double) s * (double) s;
                ++count;
            }
        }

        if (count == 0)
            return;

        const float rms = (float) std::sqrt (sumSquares / (double) count);

        // Peak-hold with decay: new peaks are instant, old values decay
        // towards the new RMS over time so the meter feels natural.
        const float candidate = juce::jmax (rms, level.load (std::memory_order_relaxed) * 0.85f);
        level.store (candidate, std::memory_order_relaxed);
    }

    // Called from the GUI thread.
    float getLevel() const noexcept { return level.load (std::memory_order_relaxed); }

    void reset() noexcept { level.store (0.0f, std::memory_order_relaxed); }

private:
    std::atomic<float> level { 0.0f };
};

} // namespace bva
