#pragma once
/*
    TestHelpers.h
    -------------
    Small deterministic helpers shared by the headless test suite.
    No external audio files, no randomness beyond a fixed-seed LCG
    used only for a real-time-safety smoke test.
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include "../Source/PluginProcessor.h"

namespace bva::test
{

inline std::unique_ptr<BerlinVocalAIAudioProcessor> makeProcessor (
    double sampleRate = 44100.0, int blockSize = 512)
{
    auto p = std::make_unique<BerlinVocalAIAudioProcessor>();
    p->prepareToPlay (sampleRate, (int) blockSize);
    return p;
}

inline std::atomic<float>* raw (BerlinVocalAIAudioProcessor& p, const char* id)
{
    auto* v = p.getAPVTS().getRawParameterValue (id);
    jassert (v != nullptr);
    return v;
}

inline void setParam (BerlinVocalAIAudioProcessor& p, const char* id, float value)
{
    raw (p, id)->store (value, std::memory_order_relaxed);
}

inline void setParamBool (BerlinVocalAIAudioProcessor& p, const char* id, bool value)
{
    raw (p, id)->store (value ? 1.0f : 0.0f, std::memory_order_relaxed);
}

// Renders a sine into a stereo buffer block-by-block through the real
// processBlock(). Returns the buffer; test then inspects the tail.
inline juce::AudioBuffer<float> renderSine (BerlinVocalAIAudioProcessor& p,
                                            double freqHz, float amplitude,
                                            double seconds, int numChannels = 2,
                                            double sampleRate = 44100.0)
{
    const int blockSize = 512;
    const int totalSamples = (int) (sampleRate * seconds);
    juce::AudioBuffer<float> buffer (numChannels, totalSamples);
    buffer.clear();

    // Index-based phase: perfectly continuous across blocks and channels,
    // zero drift, zero wrap artefacts, fully deterministic.
    const double twoPi = juce::MathConstants<double>::twoPi;

    juce::MidiBuffer midi;
    int written = 0;
    while (written < totalSamples)
    {
        const int n = juce::jmin (blockSize, totalSamples - written);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int i = 0; i < n; ++i)
            {
                const double t = (double) (written + i) / sampleRate;
                data[written + i] = amplitude * (float) std::sin (twoPi * freqHz * t);
            }
        }

        // Zero-copy view over the next n samples of every channel.
        float* chans[2] {};
        for (int ch = 0; ch < numChannels; ++ch)
            chans[ch] = buffer.getWritePointer (ch) + written;
        juce::AudioBuffer<float> chunk (chans, numChannels, n);

        p.processBlock (chunk, midi);
        written += n;
    }
    return buffer;
}

inline juce::AudioBuffer<float> renderNoiseBand (BerlinVocalAIAudioProcessor& p,
                                                 float amplitude, double seconds,
                                                 int numChannels = 2,
                                                 double sampleRate = 44100.0)
{
    // Deterministic filtered noise: fixed-seed LCG + one-pole high pass
    // around several kHz. Good enough as a sibilance-like test signal.
    const int blockSize = 512;
    const int totalSamples = (int) (sampleRate * seconds);
    juce::AudioBuffer<float> buffer (numChannels, totalSamples);
    buffer.clear();

    unsigned long seed = 123456789UL;
    auto rand11 = [&seed]
    {
        seed = seed * 6364136223846793005UL + 1442695040888963407UL;
        return (float) ((double) (seed >> 11) / (double) (1UL << 53) * 2.0 - 1.0);
    };

    juce::MidiBuffer midi;
    int written = 0;
    while (written < totalSamples)
    {
        const int n = juce::jmin (blockSize, totalSamples - written);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            float prevIn = 0.0f, prevOut = 0.0f;
            for (int i = 0; i < n; ++i)
            {
                const float white = rand11();
                const float hp = 0.85f * (prevOut + white - prevIn); // one-pole HP ~ few kHz
                prevIn = white;
                prevOut = hp;
                data[written + i] = amplitude * hp;
            }
        }
        // Zero-copy view over the next n samples of every channel.
        float* chans[2] {};
        for (int ch = 0; ch < numChannels; ++ch)
            chans[ch] = buffer.getWritePointer (ch) + written;
        juce::AudioBuffer<float> chunk (chans, numChannels, n);

        p.processBlock (chunk, midi);
        written += n;
    }
    return buffer;
}

inline double rms (const juce::AudioBuffer<float>& buffer, int skipSamples = 0)
{
    double sum = 0.0;
    long   count = 0;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const auto* data = buffer.getReadPointer (ch);
        for (int i = skipSamples; i < buffer.getNumSamples(); ++i)
        {
            sum += (double) data[i] * (double) data[i];
            ++count;
        }
    }
    return count > 0 ? std::sqrt (sum / (double) count) : 0.0;
}

inline float maxAbsDiff (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b,
                         int skipSamples = 0)
{
    float worst = 0.0f;
    const int chCount = juce::jmin (a.getNumChannels(), b.getNumChannels());
    const int n = juce::jmin (a.getNumSamples(), b.getNumSamples());
    for (int ch = 0; ch < chCount; ++ch)
        for (int i = skipSamples; i < n; ++i)
            worst = juce::jmax (worst, std::abs (a.getReadPointer (ch)[i] - b.getReadPointer (ch)[i]));
    return worst;
}

inline bool allFinite (const juce::AudioBuffer<float>& buffer)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const auto* data = buffer.getReadPointer (ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            if (! std::isfinite (data[i]))
                return false;
    }
    return true;
}

// dB ratio between two RMS measurements.
inline double rmsDb (double reference, double measured)
{
    return 20.0 * std::log10 (measured / juce::jmax (1e-12, reference));
}

} // namespace bva::test
