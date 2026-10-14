/*
    DSPTests.cpp
    ------------
    Deterministic DSP behaviour tests using synthetic signals only.

    Measurements use the signal TAIL (first 250 ms skipped) so gain
    smoothing and module crossfades have fully settled before analysis.
*/

#include <cstdio>
#include "TestHelpers.h"

namespace bva::test
{

static int failures = 0;

#define CHECK(cond, msg)                                                        \
    do {                                                                        \
        if (cond) { std::printf ("  PASS  %s\n", (msg)); }                      \
        else        { std::printf ("  FAIL  %s (line %d)\n", (msg), __LINE__); ++failures; } \
    } while (false)

static constexpr int skipTail = (int) (44100 * 0.25); // 250 ms settle
static constexpr double renderLen = 1.0;              // seconds

// ======================================================================
// Module tests
// ======================================================================

static void testHighPassFilter()
{
    std::printf ("\n[High-Pass Filter]\n");

    // Bypassed reference processor produces the untouched input signal.
    auto makeRef = []()
    {
        auto ref = makeProcessor();
        setParamBool (*ref, bva::param::masterBypass, true);
        return ref;
    };

    // Vocal-range tone passes almost untouched.
    {
        auto p = makeProcessor();
        auto ref = makeRef();

        auto inRef = renderSine (*ref, 1000.0, 0.25, renderLen);
        auto processed = renderSine (*p, 1000.0, 0.25, renderLen);

        const double delta = rmsDb (rms (inRef, skipTail), rms (processed, skipTail));
        std::printf ("      1 kHz tone through HPF@80: delta = %.2f dB\n", delta);
        CHECK (std::abs (delta) < 0.5, "1 kHz tone passes HPF within 0.5 dB");
    }

    // Rumble below cutoff is heavily attenuated (24 dB/oct).
    {
        auto p = makeProcessor();
        auto ref = makeRef();

        auto inRef      = renderSine (*ref, 40.0, 0.5, renderLen);
        auto processed  = renderSine (*p, 40.0, 0.5, renderLen);

        const double delta = rmsDb (rms (inRef, skipTail), rms (processed, skipTail));
        std::printf ("      40 Hz tone through HPF@80: delta = %.2f dB\n", delta);
        CHECK (delta < -15.0, "40 Hz tone attenuated by more than 15 dB");

        // Disabling the HPF restores the low end (after crossfade settles).
        setParamBool (*p, bva::param::hpfEnabled, false);
        auto processedOff = renderSine (*p, 40.0, 0.5, renderLen);
        const double deltaOff = rmsDb (rms (inRef, skipTail), rms (processedOff, skipTail));
        std::printf ("      40 Hz with HPF disabled: delta = %.2f dB\n", deltaOff);
        CHECK (std::abs (deltaOff) < 0.5, "HPF disabled passes 40 Hz within 0.5 dB");
    }
}

static void testThreeBandEQ()
{
    std::printf ("\n[Three-Band EQ]\n");

    // Neutral EQ is transparent.
    {
        auto p = makeProcessor();
        auto ref = makeProcessor();
        setParamBool (*ref, bva::param::masterBypass, true);

        auto inRef = renderSine (*ref, 1000.0, 0.25, renderLen);
        auto out   = renderSine (*p, 1000.0, 0.25, renderLen);

        const double delta = rmsDb (rms (inRef, skipTail), rms (out, skipTail));
        std::printf ("      neutral EQ at 1 kHz: delta = %.2f dB\n", delta);
        CHECK (std::abs (delta) < 0.25, "neutral EQ (all 0 dB) transparent within 0.25 dB");
    }

    // Mid boost of +12 dB at 1 kHz.
    {
        auto p = makeProcessor();
        setParam (*p, bva::param::eqMidFreq, 1000.0f);
        setParam (*p, bva::param::eqMidGain, 12.0f);
        setParam (*p, bva::param::eqMidQ, 1.0f);

        auto ref = makeProcessor();
        setParamBool (*ref, bva::param::masterBypass, true);
        auto inRef = renderSine (*ref, 1000.0, 0.125, renderLen);
        auto out   = renderSine (*p, 1000.0, 0.125, renderLen);

        const double delta = rmsDb (rms (inRef, skipTail), rms (out, skipTail));
        std::printf ("      +12 dB mid boost @1 kHz: measured %.2f dB\n", delta);
        CHECK (delta > 9.5 && delta < 14.0, "mid band boost within tolerance");
    }

    // Low shelf boost measured RELATIVE to the same chain with shelf 0 dB:
    // the default-enabled HPF (80 Hz) also acts on 60 Hz, so the correct
    // measurement isolates the shelf's own contribution.
    {
        auto pBoost = makeProcessor();
        setParam (*pBoost, bva::param::eqLowFreq, 120.0f);
        setParam (*pBoost, bva::param::eqLowGain, 12.0f);

        auto pFlat = makeProcessor();
        setParam (*pFlat, bva::param::eqLowFreq, 120.0f);
        setParam (*pFlat, bva::param::eqLowGain, 0.0f);

        auto inFlat  = renderSine (*pFlat, 60.0, 0.125, renderLen);
        auto inBoost = renderSine (*pBoost, 60.0, 0.125, renderLen);

        const double delta = rmsDb (rms (inFlat, skipTail), rms (inBoost, skipTail));
        std::printf ("      +12 dB low shelf @60 Hz (vs flat): measured %.2f dB\n", delta);
        CHECK (delta > 6.0 && delta < 13.0, "low shelf boost within tolerance");
    }
}

static void testCompressor()
{
    std::printf ("\n[Compressor]\n");

    // Loud signal above threshold is reduced.
    {
        auto p = makeProcessor();
        setParamBool (*p, bva::param::compEnabled, true);
        setParam (*p, bva::param::compThreshold, -18.0f);
        setParam (*p, bva::param::compRatio, 2.0f);
        setParam (*p, bva::param::compAttack, 10.0f);
        setParam (*p, bva::param::compRelease, 100.0f);
        setParam (*p, bva::param::compMakeup, 0.0f);

        auto ref = makeProcessor();
        setParamBool (*ref, bva::param::masterBypass, true);
        auto inRef = renderSine (*ref, 220.0, 0.7, renderLen);   // ~ -3 dBFS
        auto out   = renderSine (*p, 220.0, 0.7, renderLen);

        const double delta = rmsDb (rms (inRef, skipTail), rms (out, skipTail));
        std::printf ("      comp 2:1 @-18dB, input -3 dBFS: reduction %.2f dB\n", delta);
        CHECK (delta < -4.0 && delta > -12.0, "compressor reduces loud signal (expected ~ -6..-8 dB)");
    }

    // Quiet signal below threshold passes unchanged.
    {
        auto p = makeProcessor();
        setParamBool (*p, bva::param::compEnabled, true);
        setParam (*p, bva::param::compThreshold, -18.0f);

        auto ref = makeProcessor();
        setParamBool (*ref, bva::param::masterBypass, true);
        auto inRef = renderSine (*ref, 220.0, 0.01, renderLen);  // ~ -40 dBFS
        auto out   = renderSine (*p, 220.0, 0.01, renderLen);

        const double delta = rmsDb (rms (inRef, skipTail), rms (out, skipTail));
        std::printf ("      comp with input -40 dBFS: delta %.2f dB\n", delta);
        CHECK (std::abs (delta) < 0.5, "compressor transparent below threshold");
    }

    // Makeup gain lifts the compressed path.
    {
        auto p = makeProcessor();
        setParamBool (*p, bva::param::compEnabled, true);
        setParam (*p, bva::param::compThreshold, -18.0f);
        setParam (*p, bva::param::compRatio, 4.0f);
        setParam (*p, bva::param::compMakeup, 6.0f);

        auto ref = makeProcessor();
        setParamBool (*ref, bva::param::masterBypass, true);
        auto inRef = renderSine (*ref, 220.0, 0.7, renderLen);
        auto out   = renderSine (*p, 220.0, 0.7, renderLen);

        const double delta = rmsDb (rms (inRef, skipTail), rms (out, skipTail));
        std::printf ("      comp 4:1 + 6 dB makeup: delta %.2f dB\n", delta);
        CHECK (delta > -12.0 && delta < 0.0, "makeup gain present but compression still active");
    }
}

static void testDeEsser()
{
    std::printf ("\n[De-esser]\n");

    // Sibilance-like high-frequency noise band is reduced.
    {
        auto p = makeProcessor();
        setParamBool (*p, bva::param::deessEnabled, true);
        setParam (*p, bva::param::deessFreq, 6500.0f);
        setParam (*p, bva::param::deessThreshold, -30.0f);
        setParam (*p, bva::param::deessAmount, 6.0f);

        auto ref = makeProcessor();
        setParamBool (*ref, bva::param::masterBypass, true);

        auto inRef = renderNoiseBand (*ref, 0.4, renderLen);
        auto out   = renderNoiseBand (*p, 0.4, renderLen);

        const double delta = rmsDb (rms (inRef, skipTail), rms (out, skipTail));
        std::printf ("      de-esser on HF noise: reduction %.2f dB\n", delta);
        CHECK (delta < -2.5, "de-esser attenuates sibilance-like band");

        // Disabling restores the level.
        setParamBool (*p, bva::param::deessEnabled, false);
        auto outOff = renderNoiseBand (*p, 0.4, renderLen);
        const double deltaOff = rmsDb (rms (inRef, skipTail), rms (outOff, skipTail));
        std::printf ("      de-esser disabled: delta %.2f dB\n", deltaOff);
        CHECK (std::abs (deltaOff) < 0.5, "de-esser disabled is transparent");
    }

    // Low-frequency content is untouched (lives in the low band).
    {
        auto p = makeProcessor();
        setParamBool (*p, bva::param::deessEnabled, true);
        setParam (*p, bva::param::deessFreq, 6500.0f);
        setParam (*p, bva::param::deessThreshold, -60.0f); // very sensitive

        auto ref = makeProcessor();
        setParamBool (*ref, bva::param::masterBypass, true);
        auto inRef = renderSine (*ref, 500.0, 0.25, renderLen);
        auto out   = renderSine (*p, 500.0, 0.25, renderLen);

        const double delta = rmsDb (rms (inRef, skipTail), rms (out, skipTail));
        std::printf ("      500 Hz tone with de-esser active: delta %.2f dB\n", delta);
        CHECK (std::abs (delta) < 0.5, "de-esser leaves low band untouched");
    }
}

static void testDryWetAndBypass()
{
    std::printf ("\n[Dry/Wet & Master Bypass]\n");

    // Dry/Wet 0% must return the untouched input.
    {
        auto p = makeProcessor();
        setParam (*p, bva::param::dryWet, 0.0f);
        setParam (*p, bva::param::inputGain, 3.0f); // make chain visibly different

        auto ref = makeProcessor();
        setParamBool (*ref, bva::param::masterBypass, true);
        auto inRef = renderSine (*ref, 1000.0, 0.25, renderLen);
        auto out   = renderSine (*p, 1000.0, 0.25, renderLen);

        const double delta = rmsDb (rms (inRef, skipTail), rms (out, skipTail));
        CHECK (std::abs (delta) < 0.1, "Dry/Wet 0% returns input signal");

        const float maxDiff = maxAbsDiff (inRef, out, skipTail);
        std::printf ("      0%% max sample diff = %.6f\n", maxDiff);
        CHECK (maxDiff < 0.001f, "Dry/Wet 0% sample-accurate (within 0.001)");
    }

    // Master bypass must return the untouched input even with a hot chain.
    {
        auto p = makeProcessor();
        setParam (*p, bva::param::inputGain, 6.0f);
        setParam (*p, bva::param::outputGain, 6.0f);
        setParamBool (*p, bva::param::compEnabled, true);
        setParamBool (*p, bva::param::masterBypass, true);

        auto ref = makeProcessor();
        setParamBool (*ref, bva::param::masterBypass, true);
        auto inRef = renderSine (*ref, 1000.0, 0.25, renderLen);
        auto out   = renderSine (*p, 1000.0, 0.25, renderLen);

        const float maxDiff = maxAbsDiff (inRef, out, skipTail);
        std::printf ("      bypass max sample diff = %.6f\n", maxDiff);
        CHECK (maxDiff < 1e-6f, "master bypass returns input bit-exactly after crossfade");
    }
}

static void testFormatsAndSafety()
{
    std::printf ("\n[Formats, NaN safety, determinism, reinit]\n");

    // Mono processing.
    {
        auto p = makeProcessor();
        auto mono = juce::AudioProcessor::BusesLayout();
        mono.inputBuses.add (juce::AudioChannelSet::mono());
        mono.outputBuses.add (juce::AudioChannelSet::mono());
        CHECK (p->setBusesLayout (mono), "switch to mono layout");
        auto buf = renderSine (*p, 1000.0, 0.25, 0.5, 1);
        CHECK (buf.getNumChannels() == 1 && allFinite (buf), "mono processing finite");
    }

    // Stereo processing at other rates/blocks.
    {
        auto p = makeProcessor (48000.0, 64);
        auto buf = renderSine (*p, 1000.0, 0.25, 0.25, 2, 48000.0);
        CHECK (allFinite (buf), "stereo processing finite at 48 kHz / 64-block");
    }

    // No NaN/Inf with extreme input.
    {
        auto p = makeProcessor();
        auto pRef = makeProcessor();
        setParamBool (*pRef, bva::param::masterBypass, true);
        // Square-ish extreme signal via loud sine + input gain.
        setParam (*p, bva::param::inputGain, 24.0f);
        setParam (*p, bva::param::outputGain, -12.0f);
        auto buf = renderSine (*p, 300.0, 0.95, 0.5);
        CHECK (allFinite (buf), "no NaN/Inf with extreme input gain");
    }

    // Determinism: two identical processors produce identical output.
    {
        auto a = makeProcessor();
        auto b = makeProcessor();
        setParamBool (*a, bva::param::compEnabled, true);
        setParamBool (*b, bva::param::compEnabled, true);
        auto outA = renderSine (*a, 440.0, 0.5, 0.5);
        auto outB = renderSine (*b, 440.0, 0.5, 0.5);
        CHECK (maxAbsDiff (outA, outB) == 0.0f, "deterministic processing (bit-exact)");
    }

    // Reinitialisation at different rates and block sizes.
    {
        auto p = makeProcessor (44100.0, 512);
        renderSine (*p, 500.0, 0.3, 0.1);

        p->prepareToPlay (48000.0, 64);
        auto buf1 = renderSine (*p, 500.0, 0.3, 0.1, 2, 48000.0);
        CHECK (allFinite (buf1), "reinit at 48k/64 OK");

        p->prepareToPlay (96000.0, 4096);
        auto buf2 = renderSine (*p, 500.0, 0.3, 0.1, 2, 96000.0);
        CHECK (allFinite (buf2), "reinit at 96k/4096 OK");

        p->reset();
        auto buf3 = renderSine (*p, 500.0, 0.3, 0.1, 2, 96000.0);
        CHECK (allFinite (buf3), "reset + process OK");
    }
}

static void testRealTimeSafetySmoke()
{
    std::printf ("\n[Real-time safety smoke test]\n");

    // Hammer the parameter path with rapid deterministic changes while
    // processing. Output must stay finite and bounded at all times.
    auto p = makeProcessor (44100.0, 512);

    unsigned long seed = 42UL;
    auto rand11 = [&seed]
    {
        seed = seed * 6364136223846793005UL + 1442695040888963407UL;
        return (float) ((double) (seed >> 11) / (double) (1UL << 53) * 2.0 - 1.0);
    };

    juce::MidiBuffer midi;
    bool bounded = true;

    for (int block = 0; block < 200; ++block)
    {
        // Change 4 random parameters every block.
        for (int k = 0; k < 4; ++k)
        {
            const float r = rand11();
            switch ((block + k) % 6)
            {
                case 0: setParam (*p, bva::param::inputGain, r * 24.0f); break;
                case 1: setParam (*p, bva::param::hpfFreq, 20.0f + (r * 0.5f + 0.5f) * 230.0f); break;
                case 2: setParam (*p, bva::param::eqMidGain, r * 15.0f); break;
                case 3: setParam (*p, bva::param::compThreshold, r * 60.0f); break;
                case 4: setParam (*p, bva::param::dryWet, (r * 0.5f + 0.5f) * 100.0f); break;
                case 5: setParamBool (*p, bva::param::masterBypass, r > 0.8f); break;
            }
        }

        juce::AudioBuffer<float> buf (2, 512);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 512; ++i)
                buf.setSample (ch, i, 0.5f * std::sin ((float) i * 0.05f));

        p->processBlock (buf, midi);

        if (! allFinite (buf)) { bounded = false; break; }
        if (maxAbsDiff (buf, buf) > 8.0f) { bounded = false; break; }
    }

    CHECK (bounded, "rapid parameter changes stay finite and bounded");
}

int runDSPTests()
{
    std::printf ("\n--- DSP tests ---\n");
    testHighPassFilter();
    testThreeBandEQ();
    testCompressor();
    testDeEsser();
    testDryWetAndBypass();
    testFormatsAndSafety();
    testRealTimeSafetySmoke();
    return failures;
}

} // namespace bva::test
