#include "Parameters.h"

namespace bva
{

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using PFloat = juce::AudioParameterFloat;
    using PBool  = juce::AudioParameterBool;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ------------------------------------------------------------------
    // Helper lambdas keep the ranges explicit and consistent.
    // ------------------------------------------------------------------
    auto addFloat = [&layout] (const juce::String& id, const juce::String& name,
                               float minV, float maxV, float inc, float skew,
                               float defaultV, const juce::String& unit)
    {
        // All defaults above are inside their ranges, so clamping of the
        // default value is never an issue. Ranges are explicit on purpose.
        juce::NormalisableRange<float> range (minV, maxV, inc, skew);
        layout.add (std::make_unique<PFloat> (id, name, range, defaultV, unit,
                                              juce::AudioProcessorParameter::genericParameter,
                                              /*stringFromValue*/ nullptr,
                                              /*valueFromString*/ nullptr));
    };

    auto addBool = [&layout] (const juce::String& id, const juce::String& name, bool defaultV)
    {
        layout.add (std::make_unique<PBool> (id, name, defaultV));
    };

    // ---- Input --------------------------------------------------------
    addFloat (param::inputGain, "Input Gain", -24.0f, 24.0f, 0.1f, 1.0f,
              ParameterDefaults::inputGainDb, "dB");

    // ---- High-pass filter --------------------------------------------
    addBool  (param::hpfEnabled, "High-Pass Enabled", ParameterDefaults::hpfEnabledOn);
    addFloat (param::hpfFreq, "High-Pass Cutoff", 20.0f, 250.0f, 1.0f, 0.35f,
              ParameterDefaults::hpfFreqHz, "Hz");

    // ---- Three-band parametric EQ ------------------------------------
    // Frequency ranges overlap deliberately so the user cannot create an
    // impossible band arrangement, and each band can reach its neighbours.
    addFloat (param::eqLowFreq,  "Low Freq",  40.0f,   500.0f,  1.0f, 0.30f, ParameterDefaults::eqLowFreqHz,  "Hz");
    addFloat (param::eqLowGain,  "Low Gain",  -15.0f,  15.0f,   0.1f, 1.0f,  ParameterDefaults::eqLowGainDb,  "dB");
    addFloat (param::eqLowQ,     "Low Q",     0.1f,    10.0f,   0.01f, 0.35f, ParameterDefaults::eqLowQ,     "");

    addFloat (param::eqMidFreq,  "Mid Freq",  200.0f,  5000.0f, 1.0f, 0.30f, ParameterDefaults::eqMidFreqHz,  "Hz");
    addFloat (param::eqMidGain,  "Mid Gain",  -15.0f,  15.0f,   0.1f, 1.0f,  ParameterDefaults::eqMidGainDb,  "dB");
    addFloat (param::eqMidQ,     "Mid Q",     0.1f,    10.0f,   0.01f, 0.35f, ParameterDefaults::eqMidQ,     "");

    addFloat (param::eqHighFreq, "High Freq", 2000.0f, 16000.0f, 1.0f, 0.30f, ParameterDefaults::eqHighFreqHz, "Hz");
    addFloat (param::eqHighGain, "High Gain", -15.0f,  15.0f,   0.1f, 1.0f,  ParameterDefaults::eqHighGainDb, "dB");
    addFloat (param::eqHighQ,    "High Q",    0.1f,    10.0f,   0.01f, 0.35f, ParameterDefaults::eqHighQ,    "");

    // ---- Compressor ---------------------------------------------------
    addBool  (param::compEnabled, "Compressor Enabled", ParameterDefaults::compEnabledOn);
    addFloat (param::compThreshold, "Comp Threshold", -60.0f, 0.0f, 0.1f, 1.0f,
              ParameterDefaults::compThresholdDb, "dB");
    addFloat (param::compRatio,  "Comp Ratio", 1.0f, 20.0f, 0.1f, 0.5f,
              ParameterDefaults::compRatio, ":1");
    addFloat (param::compAttack, "Comp Attack", 0.5f, 200.0f, 0.1f, 0.35f,
              ParameterDefaults::compAttackMs, "ms");
    addFloat (param::compRelease, "Comp Release", 5.0f, 1000.0f, 1.0f, 0.35f,
              ParameterDefaults::compReleaseMs, "ms");
    addFloat (param::compMakeup, "Comp Makeup", 0.0f, 24.0f, 0.1f, 1.0f,
              ParameterDefaults::compMakeupDb, "dB");

    // ---- De-esser (simple split-band prototype) -----------------------
    addBool  (param::deessEnabled, "De-esser Enabled", ParameterDefaults::deessEnabledOn);
    addFloat (param::deessFreq, "De-esser Freq", 3000.0f, 12000.0f, 10.0f, 0.3f,
              ParameterDefaults::deessFreqHz, "Hz");
    addFloat (param::deessThreshold, "De-esser Threshold", -60.0f, 0.0f, 0.1f, 1.0f,
              ParameterDefaults::deessThresholdDb, "dB");
    addFloat (param::deessAmount, "De-esser Amount", 0.0f, 24.0f, 0.1f, 1.0f,
              ParameterDefaults::deessAmountDb, "dB");

    // ---- Output / dry-wet / master bypass -----------------------------
    addFloat (param::outputGain, "Output Gain", -24.0f, 24.0f, 0.1f, 1.0f,
              ParameterDefaults::outputGainDb, "dB");
    addFloat (param::dryWet, "Dry/Wet", 0.0f, 100.0f, 1.0f, 1.0f,
              ParameterDefaults::dryWetPct, "%");
    addBool  (param::masterBypass, "Master Bypass", ParameterDefaults::masterBypassOn);

    return layout;
}

ParameterValues snapshotValues (const juce::AudioProcessorValueTreeState& apvts)
{
    auto raw = [&apvts] (const char* id) -> float
    {
        auto* p = apvts.getRawParameterValue (id);
        jassert (p != nullptr);
        return p != nullptr ? p->load (std::memory_order_relaxed) : 0.0f;
    };

    auto asBool = [] (float v) { return v > 0.5f; };

    ParameterValues v;
    v.inputGainDb      = raw (param::inputGain);
    v.hpfEnabled       = asBool (raw (param::hpfEnabled));
    v.hpfFreqHz        = raw (param::hpfFreq);

    v.eqLowFreqHz      = raw (param::eqLowFreq);
    v.eqLowGainDb      = raw (param::eqLowGain);
    v.eqLowQ           = raw (param::eqLowQ);
    v.eqMidFreqHz      = raw (param::eqMidFreq);
    v.eqMidGainDb      = raw (param::eqMidGain);
    v.eqMidQ           = raw (param::eqMidQ);
    v.eqHighFreqHz     = raw (param::eqHighFreq);
    v.eqHighGainDb     = raw (param::eqHighGain);
    v.eqHighQ          = raw (param::eqHighQ);

    v.compEnabled      = asBool (raw (param::compEnabled));
    v.compThresholdDb  = raw (param::compThreshold);
    v.compRatio        = raw (param::compRatio);
    v.compAttackMs     = raw (param::compAttack);
    v.compReleaseMs    = raw (param::compRelease);
    v.compMakeupDb     = raw (param::compMakeup);

    v.deessEnabled     = asBool (raw (param::deessEnabled));
    v.deessFreqHz      = raw (param::deessFreq);
    v.deessThresholdDb = raw (param::deessThreshold);
    v.deessAmountDb    = raw (param::deessAmount);

    v.outputGainDb     = raw (param::outputGain);
    v.dryWetPct        = raw (param::dryWet);
    v.masterBypass     = asBool (raw (param::masterBypass));
    return v;
}

} // namespace bva
