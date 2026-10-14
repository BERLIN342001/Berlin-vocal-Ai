#pragma once
/*
    VocalProcessor.h
    ----------------
    The complete Phase 1 vocal chain, owned by PluginProcessor and driven
    from its processBlock(). Audio-thread only.

    Chain (stereo-linked parameters, per-channel processing):

        input -> InputGain -> [HPF] -> EQ -> [Compressor] -> [DeEsser]
              -> OutputGain -> Dry/Wet -> (Master Bypass) -> out

    [x] modules process continuously; their enable toggles crossfade
    between the processed and unprocessed paths (10 ms) so toggling is
    click-free. The Master Bypass crossfades the WHOLE chain back to the
    untouched input signal (50 ms).

    Real-time safety contract:
    - No heap allocation, no locks, no file/network I/O here.
    - All buffers/state are allocated in prepareToPlay().
    - Parameter values arrive as a plain POD snapshot per block.
*/

#include "../Parameters.h"
#include "ThreeBandEQ.h"
#include "VocalCompressor.h"
#include "DeEsser.h"
#include "LevelMeter.h"

namespace bva
{

class VocalProcessor
{
public:
    static constexpr int maxChannels = 2;

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // Called once per block before processing.
    void setParameterValues (const ParameterValues& v);

    // Real-time safe. buffer is modified in place.
    void process (juce::AudioBuffer<float>& buffer);

    // Meters (GUI reads these via atomics).
    LevelMeterSource inputMeter;
    LevelMeterSource outputMeter;

    // Unit tests need the exact chain behaviour.
    const ThreeBandEQ&    getEQ()        const noexcept { return eq; }
    const VocalCompressor& getCompressor() const noexcept { return compressor; }
    const DeEsser&        getDeEsser()   const noexcept { return deEsser; }

private:
    // Ramped gain stages (per-sample smoothing; block-rate targets).
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> inputGain;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGain;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> makeupGain;

    ThreeBandEQ     eq;
    VocalCompressor compressor;
    DeEsser         deEsser;

    // Two cascaded 2nd-order high-pass sections = 24 dB/oct vocal HPF.
    // Indexing: hpf[section][channel].
    std::array<std::array<juce::dsp::IIR::Filter<float>, maxChannels>, 2> hpf;

    // Enable crossfades (10 ms) for the switchable modules.
    // Index: 0 = HPF, 1 = compressor, 2 = de-esser.
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 3> moduleGains;

    // Dry/wet + master bypass crossfades.
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> wetMix;    // 0..1
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> bypassMix; // 0..1 (1 = bypassed)

    double currentSampleRate = 44100.0;
    int    currentMaxBlock   = 512;

    ParameterValues params;
    bool            paramsValid = false;
};

} // namespace bva
