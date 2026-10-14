/*
    ParameterTests.cpp
    ------------------
    Parameter layout, defaults, ranges, host state save/restore,
    and bus layout rules.
*/

#include <cstdio>
#include "TestHelpers.h"

namespace bva::test
{

static int failures = 0;

#define CHECK(cond, msg)                                                        \
    do {                                                                        \
        if (cond) { std::printf ("  PASS  %s\n", (msg)); }                      \
        else        { std::printf ("  FAIL  %s\n", (msg)); ++failures; }        \
    } while (false)

int runParameterTests()
{
    std::printf ("\n--- Parameter & state tests ---\n");

    auto p = makeProcessor();

    // 1. Defaults (Phase 1 brief, section 5).
    //    Comparisons use small tolerances: skewed parameter ranges round-trip
    //    through normalised space, and interval snapping (e.g. Q steps of
    //    0.01) may nudge a default by less than one step.
    auto v = bva::snapshotValues (p->getAPVTS());
    auto nearEq = [] (float a, float b, float tol) { return std::abs (a - b) <= tol; };

    CHECK (nearEq (v.inputGainDb, bva::ParameterDefaults::inputGainDb, 0.01f), "default Input Gain = 0 dB");
    CHECK (v.hpfEnabled == bva::ParameterDefaults::hpfEnabledOn, "default HPF enabled");
    CHECK (nearEq (v.hpfFreqHz, bva::ParameterDefaults::hpfFreqHz, 0.01f), "default HPF cutoff = 80 Hz");
    CHECK (nearEq (v.eqLowFreqHz, bva::ParameterDefaults::eqLowFreqHz, 0.01f), "default Low Freq = 120 Hz");
    CHECK (nearEq (v.eqMidFreqHz, bva::ParameterDefaults::eqMidFreqHz, 0.01f), "default Mid Freq = 1 kHz");
    CHECK (nearEq (v.eqHighFreqHz, bva::ParameterDefaults::eqHighFreqHz, 0.01f), "default High Freq = 8 kHz");
    CHECK (nearEq (v.eqLowQ, bva::ParameterDefaults::eqLowQ, 0.02f), "default Low Q ~ 0.707");
    CHECK (nearEq (v.eqMidQ, bva::ParameterDefaults::eqMidQ, 0.02f), "default Mid Q ~ 0.707");
    CHECK (nearEq (v.eqHighQ, bva::ParameterDefaults::eqHighQ, 0.02f), "default High Q ~ 0.707");
    CHECK (v.compEnabled == bva::ParameterDefaults::compEnabledOn, "default Compressor disabled");
    CHECK (nearEq (v.compThresholdDb, bva::ParameterDefaults::compThresholdDb, 0.01f), "default Comp Threshold = -18 dB");
    CHECK (nearEq (v.compRatio, bva::ParameterDefaults::compRatio, 0.01f), "default Comp Ratio = 2:1");
    CHECK (v.deessEnabled == bva::ParameterDefaults::deessEnabledOn, "default De-esser disabled");
    CHECK (nearEq (v.outputGainDb, bva::ParameterDefaults::outputGainDb, 0.01f), "default Output Gain = 0 dB");
    CHECK (nearEq (v.dryWetPct, bva::ParameterDefaults::dryWetPct, 0.01f), "default Dry/Wet = 100%");
    CHECK (v.masterBypass == bva::ParameterDefaults::masterBypassOn, "default Bypass off");

    // 2. All stable parameter IDs exist; host sees exactly that many.
    {
        const char* allIds[] =
        {
            bva::param::inputGain, bva::param::hpfEnabled, bva::param::hpfFreq,
            bva::param::eqLowFreq, bva::param::eqLowGain, bva::param::eqLowQ,
            bva::param::eqMidFreq, bva::param::eqMidGain, bva::param::eqMidQ,
            bva::param::eqHighFreq, bva::param::eqHighGain, bva::param::eqHighQ,
            bva::param::compEnabled, bva::param::compThreshold, bva::param::compRatio,
            bva::param::compAttack, bva::param::compRelease, bva::param::compMakeup,
            bva::param::deessEnabled, bva::param::deessFreq, bva::param::deessThreshold,
            bva::param::deessAmount, bva::param::outputGain, bva::param::dryWet,
            bva::param::masterBypass
        };

        int missing = 0;
        for (auto* id : allIds)
            if (p->getAPVTS().getRawParameterValue (id) == nullptr)
            {
                std::printf ("      missing parameter ID: %s\n", id);
                ++missing;
            }

        CHECK (missing == 0, "all 25 stable parameter IDs resolvable");
        CHECK (p->getParameters().size() == 25, "exactly 25 parameters exposed to host");
    }

    // 3. Range clamping via the normalised host interface.
    //    Value above maximum must snap to the parameter range.
    {
        auto* inGain = p->getAPVTS().getParameter (bva::param::inputGain);
        inGain->setValueNotifyingHost (2.0f);           // > 1 -> clamps to max
        inGain->setValueNotifyingHost (inGain->convertTo0to1 (999.0f));
        std::printf ("  (inputGain after out-of-range set: %.2f dB)\n",
                     raw (*p, "inputGain")->load());
        CHECK (raw (*p, "inputGain")->load() <= 24.0f + 1e-3f, "out-of-range value clamped to max");
        CHECK (raw (*p, "inputGain")->load() >= -24.0f - 1e-3f, "out-of-range value clamped to min");
    }

    // 4. State save / restore round-trip through the HOST mechanism.
    {
        // Bring the processor back to a known state first (test 3 left
        // inputGain clamped at its maximum).
        setParam (*p, bva::param::inputGain, 0.0f);

        juce::MemoryBlock saved;
        p->getStateInformation (saved);
        CHECK (saved.getSize() > 0, "getStateInformation produces data");

        // Change several parameters...
        setParam (*p, bva::param::inputGain, -6.0f);
        setParam (*p, bva::param::hpfFreq, 150.0f);
        setParamBool (*p, bva::param::compEnabled, true);
        setParam (*p, bva::param::compThreshold, -30.0f);
        setParam (*p, bva::param::dryWet, 40.0f);
        setParamBool (*p, bva::param::masterBypass, true);

        // ...restore...
        p->setStateInformation (saved.getData(), (int) saved.getSize());

        // ...and verify everything came back.
        auto v2 = bva::snapshotValues (p->getAPVTS());
        CHECK (v2.inputGainDb == bva::ParameterDefaults::inputGainDb, "restore: inputGain back to default");
        CHECK (v2.hpfFreqHz == bva::ParameterDefaults::hpfFreqHz, "restore: hpfFreq back to default");
        CHECK (v2.compEnabled == bva::ParameterDefaults::compEnabledOn, "restore: compEnabled back to default");
        CHECK (v2.compThresholdDb == bva::ParameterDefaults::compThresholdDb, "restore: compThreshold back to default");
        CHECK (v2.dryWetPct == bva::ParameterDefaults::dryWetPct, "restore: dryWet back to default");
        CHECK (v2.masterBypass == bva::ParameterDefaults::masterBypassOn, "restore: masterBypass back to default");
    }

    // 5. Corrupted state must not crash.
    {
        auto before = bva::snapshotValues (p->getAPVTS());

        const char garbage[] = "this is not valid plugin state";
        p->setStateInformation (garbage, (int) sizeof (garbage));
        CHECK (true, "corrupt state handled without crashing");

        auto after = bva::snapshotValues (p->getAPVTS());
        CHECK (std::abs (after.inputGainDb - before.inputGainDb) < 0.001f
                   && after.hpfEnabled == before.hpfEnabled,
               "state unchanged after corrupt blob");
    }

    // 6. Bus layouts: mono and stereo supported, others rejected.
    {
        auto mono  = juce::AudioChannelSet::mono();
        auto monoBoth = juce::AudioProcessor::BusesLayout();
        monoBoth.inputBuses.add (mono);
        monoBoth.outputBuses.add (mono);
        CHECK (p->checkBusesLayoutSupported (monoBoth), "mono -> mono supported");

        auto stereo = juce::AudioChannelSet::stereo();
        auto stereoBoth = juce::AudioProcessor::BusesLayout();
        stereoBoth.inputBuses.add (stereo);
        stereoBoth.outputBuses.add (stereo);
        CHECK (p->checkBusesLayoutSupported (stereoBoth), "stereo -> stereo supported");

        auto mixed = juce::AudioProcessor::BusesLayout();
        mixed.inputBuses.add (mono);
        mixed.outputBuses.add (stereo);
        CHECK (! p->checkBusesLayoutSupported (mixed), "mono -> stereo rejected");

        auto surround = juce::AudioProcessor::BusesLayout();
        surround.inputBuses.add (juce::AudioChannelSet::quadraphonic());
        surround.outputBuses.add (juce::AudioChannelSet::quadraphonic());
        CHECK (! p->checkBusesLayoutSupported (surround), "quad -> quad rejected");
    }

    // 7. No reported latency (Phase 1 chain has none).
    CHECK (p->getLatencySamples() == 0, "reported latency is 0 samples");

    return failures;
}

} // namespace bva::test
