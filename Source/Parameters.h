#pragma once
/*
    Parameters.h
    ------------
    Central parameter definition for Berlin Vocal AI (Phase 1).

    All parameter IDs are stable strings. DO NOT rename an ID after a
    release: saved host sessions and automation data reference these
    strings. If a parameter must change meaning, add a NEW id and
    migrate using parameter version hints.

    Ranges/defaults follow the Phase 1 brief (section 5). They are
    engineering defaults, not "vocal presets".
*/

#include <juce_audio_processors/juce_audio_processors.h>

namespace bva
{

// Stable parameter ID strings (referenced by DSP, GUI and tests).
namespace param
{
    constexpr const char* inputGain    = "inputGain";
    constexpr const char* hpfEnabled   = "hpfEnabled";
    constexpr const char* hpfFreq      = "hpfFreq";
    constexpr const char* eqLowFreq    = "eqLowFreq";
    constexpr const char* eqLowGain    = "eqLowGain";
    constexpr const char* eqLowQ       = "eqLowQ";
    constexpr const char* eqMidFreq    = "eqMidFreq";
    constexpr const char* eqMidGain    = "eqMidGain";
    constexpr const char* eqMidQ       = "eqMidQ";
    constexpr const char* eqHighFreq   = "eqHighFreq";
    constexpr const char* eqHighGain   = "eqHighGain";
    constexpr const char* eqHighQ      = "eqHighQ";
    constexpr const char* compEnabled  = "compEnabled";
    constexpr const char* compThreshold = "compThreshold";
    constexpr const char* compRatio    = "compRatio";
    constexpr const char* compAttack   = "compAttack";
    constexpr const char* compRelease  = "compRelease";
    constexpr const char* compMakeup   = "compMakeup";
    constexpr const char* deessEnabled = "deessEnabled";
    constexpr const char* deessFreq    = "deessFreq";
    constexpr const char* deessThreshold = "deessThreshold";
    constexpr const char* deessAmount  = "deessAmount";
    constexpr const char* outputGain   = "outputGain";
    constexpr const char* dryWet       = "dryWet";
    constexpr const char* masterBypass = "masterBypass";
} // namespace param

// Default values (Phase 1 brief, section 5).
struct ParameterDefaults
{
    static constexpr float inputGainDb     = 0.0f;
    static constexpr bool  hpfEnabledOn    = true;
    static constexpr float hpfFreqHz       = 80.0f;
    static constexpr float eqLowFreqHz     = 120.0f;
    static constexpr float eqLowGainDb     = 0.0f;
    static constexpr float eqLowQ          = 0.707f;
    static constexpr float eqMidFreqHz     = 1000.0f;
    static constexpr float eqMidGainDb     = 0.0f;
    static constexpr float eqMidQ          = 0.707f;
    static constexpr float eqHighFreqHz    = 8000.0f;
    static constexpr float eqHighGainDb    = 0.0f;
    static constexpr float eqHighQ         = 0.707f;
    static constexpr bool  compEnabledOn   = false;
    static constexpr float compThresholdDb = -18.0f;
    static constexpr float compRatio       = 2.0f;
    static constexpr float compAttackMs    = 10.0f;
    static constexpr float compReleaseMs   = 100.0f;
    static constexpr float compMakeupDb    = 0.0f;
    static constexpr bool  deessEnabledOn  = false;
    static constexpr float deessFreqHz     = 6500.0f;
    static constexpr float deessThresholdDb = -30.0f;
    static constexpr float deessAmountDb   = 6.0f;
    static constexpr float outputGainDb    = 0.0f;
    static constexpr float dryWetPct       = 100.0f;
    static constexpr bool  masterBypassOn  = false;
};

// Builds the AudioProcessorValueTreeState layout. Called once by the
// plugin constructor; kept here so DSP/GUI/tests share one definition.
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

// Convenience accessors used by the audio engine and the tests.
// These read the raw atomic values (real-time safe).
struct ParameterValues
{
    float inputGainDb  = 0.0f;
    bool  hpfEnabled   = true;
    float hpfFreqHz    = 80.0f;

    float eqLowFreqHz  = 120.0f;
    float eqLowGainDb  = 0.0f;
    float eqLowQ       = 0.707f;
    float eqMidFreqHz  = 1000.0f;
    float eqMidGainDb  = 0.0f;
    float eqMidQ       = 0.707f;
    float eqHighFreqHz = 8000.0f;
    float eqHighGainDb = 0.0f;
    float eqHighQ      = 0.707f;

    bool  compEnabled   = false;
    float compThresholdDb = -18.0f;
    float compRatio     = 2.0f;
    float compAttackMs  = 10.0f;
    float compReleaseMs = 100.0f;
    float compMakeupDb  = 0.0f;

    bool  deessEnabled   = false;
    float deessFreqHz    = 6500.0f;
    float deessThresholdDb = -30.0f;
    float deessAmountDb  = 6.0f;

    float outputGainDb = 0.0f;
    float dryWetPct    = 100.0f;
    bool  masterBypass = false;
};

// Snapshot all parameter values from the APVTS (real-time safe: the
// values are plain atomics; the returned struct is a plain POD copy).
ParameterValues snapshotValues (const juce::AudioProcessorValueTreeState& apvts);

} // namespace bva
