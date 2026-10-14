#include "VocalProcessor.h"

namespace bva
{

void VocalProcessor::prepare (double sampleRate, int maxBlockSize)
{
    jassert (sampleRate > 0.0);
    jassert (maxBlockSize > 0);

    currentSampleRate = sampleRate;
    currentMaxBlock   = maxBlockSize;

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlockSize, 1 };
    juce::ignoreUnused (spec);

    inputGain .reset (sampleRate, 0.020); // 20 ms gain ramps
    outputGain.reset (sampleRate, 0.020);
    makeupGain.reset (sampleRate, 0.020);
    inputGain .setCurrentAndTargetValue (1.0f);
    outputGain.setCurrentAndTargetValue (1.0f);
    makeupGain.setCurrentAndTargetValue (1.0f);

    eq.prepare (sampleRate, maxChannels);
    compressor.prepare (sampleRate, maxChannels);
    deEsser.prepare (sampleRate, maxChannels);

    for (auto& section : hpf)
        for (auto& f : section)
        {
            f.prepare ({ sampleRate, 1, 1 });
            f.reset();
        }

    for (auto& g : moduleGains)
    {
        g.reset (sampleRate, 0.010); // 10 ms module toggles
        g.setCurrentAndTargetValue (1.0f);
    }

    wetMix.reset (sampleRate, 0.030); // 30 ms dry/wet
    wetMix.setCurrentAndTargetValue (1.0f);

    bypassMix.reset (sampleRate, 0.050); // 50 ms master bypass
    bypassMix.setCurrentAndTargetValue (0.0f);

    inputMeter.reset();
    outputMeter.reset();

    // Re-apply last known parameters against the new rate/block setup.
    if (paramsValid)
        setParameterValues (params);
}

void VocalProcessor::reset()
{
    // SmoothedValue has no no-arg reset(): restore the CURRENT target
    // instantly (no ramp) so the next block continues from the right gain.
    auto restoreGain = [] (juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>& sv,
                           float db)
    {
        sv.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (db));
    };

    restoreGain (inputGain,  paramsValid ? params.inputGainDb  : 0.0f);
    restoreGain (outputGain, paramsValid ? params.outputGainDb : 0.0f);
    restoreGain (makeupGain, paramsValid ? params.compMakeupDb : 0.0f);

    eq.reset();
    compressor.reset();
    deEsser.reset();
    for (auto& section : hpf)
        for (auto& f : section)
            f.reset();
    inputMeter.reset();
    outputMeter.reset();
}

void VocalProcessor::setParameterValues (const ParameterValues& v)
{
    params = v;
    paramsValid = true;

    inputGain .setTargetValue (juce::Decibels::decibelsToGain (juce::jlimit (-24.0f, 24.0f, v.inputGainDb)));
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (juce::jlimit (-24.0f, 24.0f, v.outputGainDb)));
    makeupGain.setTargetValue (juce::Decibels::decibelsToGain (juce::jlimit (0.0f, 24.0f, v.compMakeupDb)));

    // High-pass: two cascaded 2nd-order sections at the same cutoff.
    const float hpfFreq = juce::jlimit (20.0f, 250.0f, v.hpfFreqHz);
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (currentSampleRate, hpfFreq);
    for (auto& section : hpf)
        for (auto& f : section)
            *f.coefficients = *coeffs;

    EQBandParams low  { v.eqLowFreqHz,  v.eqLowGainDb,  v.eqLowQ  };
    EQBandParams mid  { v.eqMidFreqHz,  v.eqMidGainDb,  v.eqMidQ  };
    EQBandParams high { v.eqHighFreqHz, v.eqHighGainDb, v.eqHighQ };
    eq.updateCoefficients (low, mid, high);

    compressor.setParameters (v.compThresholdDb, v.compRatio, v.compAttackMs, v.compReleaseMs);
    deEsser.setParameters (v.deessFreqHz, v.deessThresholdDb, v.deessAmountDb);

    moduleGains[0].setTargetValue (v.hpfEnabled     ? 1.0f : 0.0f);
    moduleGains[1].setTargetValue (v.compEnabled    ? 1.0f : 0.0f);
    moduleGains[2].setTargetValue (v.deessEnabled   ? 1.0f : 0.0f);

    wetMix    .setTargetValue (juce::jlimit (0.0f, 1.0f, v.dryWetPct * 0.01f));
    bypassMix .setTargetValue (v.masterBypass ? 1.0f : 0.0f);
}

void VocalProcessor::process (juce::AudioBuffer<float>& buffer)
{
    jassert (paramsValid);

    const int numChannels = juce::jmin (buffer.getNumChannels(), maxChannels);
    const int numSamples  = buffer.getNumSamples();

    if (numChannels <= 0 || numSamples <= 0)
        return;

    // --- input meter (before any processing) -------------------------
    {
        const float* chans[maxChannels] {};
        for (int ch = 0; ch < numChannels; ++ch)
            chans[ch] = buffer.getReadPointer (ch);
        inputMeter.update (chans, numChannels, numSamples);
    }

    // --- per-sample processing ---------------------------------------
    // Both channels are processed inside ONE sample loop so that all
    // smoothed crossfades advance exactly once per sample and stay
    // aligned across channels (stereo-linked behaviour).
    {
        float* data[maxChannels] {};
        for (int ch = 0; ch < numChannels; ++ch)
            data[ch] = buffer.getWritePointer (ch);

        for (int i = 0; i < numSamples; ++i)
        {
            // Advance block-rate smoothings once per sample.
            const float hpfG   = moduleGains[0].getNextValue();
            const float compG  = moduleGains[1].getNextValue();
            const float deessG = moduleGains[2].getNextValue();
            const float wMix   = wetMix   .getNextValue();
            const float bypG   = bypassMix.getNextValue();
            const float inG    = inputGain .getNextValue();
            const float outG   = outputGain.getNextValue();
            const float mkG    = makeupGain.getNextValue();

            for (int ch = 0; ch < numChannels; ++ch)
            {
                const float dry = data[ch][i];

                // 1) input gain
                float y = dry * inG;

                // 2) high-pass (24 dB/oct, two cascaded sections), crossfaded
                float hx = hpf[0][ch].processSample (y);
                hx = hpf[1][ch].processSample (hx);
                y = hpfG * hx + (1.0f - hpfG) * y;

                // 3) three-band parametric EQ (always engaged; 0 dB = neutral)
                y = eq.processSample (ch, y);

                // 4) compressor, crossfaded; makeup applied inside the
                //    compressed path only
                float cx = compressor.processSample (ch, y) * mkG;
                y = compG * cx + (1.0f - compG) * y;

                // 5) de-esser, crossfaded
                float dx = deEsser.processSample (ch, y);
                y = deessG * dx + (1.0f - deessG) * y;

                // 6) output gain
                y *= outG;

                // 7) dry/wet (equal-gain crossfade)
                y = wMix * y + (1.0f - wMix) * dry;

                // 8) master bypass (crossfades the whole chain to input)
                data[ch][i] = bypG * dry + (1.0f - bypG) * y;
            }
        }
    }

    // --- output meter -------------------------------------------------
    {
        const float* chans[maxChannels] {};
        for (int ch = 0; ch < numChannels; ++ch)
            chans[ch] = buffer.getReadPointer (ch);
        outputMeter.update (chans, numChannels, numSamples);
    }

    // --- defensive: never emit NaN/Inf --------------------------------
    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
            if (! std::isfinite (data[i]))
                data[i] = 0.0f;
    }
}

} // namespace bva
