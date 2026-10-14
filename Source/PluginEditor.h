#pragma once
/*
    PluginEditor.h
    --------------
    Phase 1 GUI. Every control is bound to a real parameter through a
    JUCE attachment, so host automation and restored state always stay
    in sync. There are no decorative/fake controls.

    Layout follows the audio signal flow top-to-bottom:
        Header (name + phase + master bypass)
        Input (gain + input meter)
        High-Pass (toggle + cutoff)
        EQ (low/mid/high: freq, gain, Q)
        Compressor (toggle + threshold, ratio, attack, release, makeup)
        De-esser (toggle + freq, threshold, amount)
        Output (gain + output meter + dry/wet)
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace bva
{

// Vertical stereo level meter reading the audio-thread atomics.
class LevelMeterComponent : public juce::Component,
                            private juce::Timer
{
public:
    LevelMeterComponent (const LevelMeterSource& sourceToRead, const juce::String& name);
    ~LevelMeterComponent() override;

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    const LevelMeterSource& source;
    float displayedLevel = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeterComponent)
};

class BerlinVocalAIAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         private juce::Timer
{
public:
    explicit BerlinVocalAIAudioProcessorEditor (BerlinVocalAIAudioProcessor&);
    ~BerlinVocalAIAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    BerlinVocalAIAudioProcessor& processorRef;
    juce::AudioProcessorValueTreeState& apvts;

    // Slider/button construction helpers (declared before attachments:
    // attachments must be destroyed before the controls they attach to).
    struct RotaryControl
    {
        juce::Slider slider;
        juce::Label  label;

        RotaryControl (const juce::String& labelText, const juce::String& tooltipText);
    };

    struct ToggleControl
    {
        juce::ToggleButton button;

        explicit ToggleControl (const juce::String& labelText);
    };

    // --- controls ------------------------------------------------------
    ToggleControl hpfEnabled   { "HPF" };
    ToggleControl compEnabled  { "Comp" };
    ToggleControl deessEnabled { "De-ess" };
    ToggleControl masterBypass { "BYPASS" };

    RotaryControl inputGain  { "Input",  "Input gain before any processing (-24..+24 dB)" };
    RotaryControl outputGain { "Output", "Output gain after the full chain (-24..+24 dB)" };
    RotaryControl dryWet     { "Dry/Wet","Blend between dry input and processed signal (0..100%)" };

    RotaryControl hpfFreq { "Cutoff", "High-pass cutoff frequency (20..250 Hz). Removes rumble below the vocal range." };

    RotaryControl eqLowFreq  { "Freq", "Low shelf corner frequency (40..500 Hz)" };
    RotaryControl eqLowGain  { "Gain", "Low shelf gain (-15..+15 dB)" };
    RotaryControl eqLowQ     { "Q",    "Low shelf slope/sharpness" };
    RotaryControl eqMidFreq  { "Freq", "Mid band centre frequency (200..5000 Hz)" };
    RotaryControl eqMidGain  { "Gain", "Mid band gain (-15..+15 dB). Useful for presence or boxiness control." };
    RotaryControl eqMidQ     { "Q",    "Mid band bandwidth" };
    RotaryControl eqHighFreq { "Freq", "High shelf corner frequency (2..16 kHz)" };
    RotaryControl eqHighGain { "Gain", "High shelf gain (-15..+15 dB). Air and brightness." };
    RotaryControl eqHighQ    { "Q",    "High shelf slope/sharpness" };

    RotaryControl compThreshold { "Thresh", "Compression starts above this level (-60..0 dB)" };
    RotaryControl compRatio     { "Ratio",  "Compression amount above threshold (1:1..20:1)" };
    RotaryControl compAttack    { "Atk",    "Compressor attack time (0.5..200 ms)" };
    RotaryControl compRelease   { "Rel",    "Compressor release time (5..1000 ms)" };
    RotaryControl compMakeup    { "Makeup", "Makeup gain applied to the compressed path (0..24 dB)" };

    RotaryControl deessFreq      { "Freq", "De-esser crossover frequency (3..12 kHz). Sibilance region." };
    RotaryControl deessThreshold { "Thresh", "De-esser reacts above this level in the high band" };
    RotaryControl deessAmount    { "Amount", "Maximum high-band attenuation (0..24 dB)" };

    // --- attachments (member order matters: declared AFTER controls) ---
    juce::AudioProcessorValueTreeState::SliderAttachment dryWetAttachment { apvts, "dryWet", dryWet.slider };
    juce::AudioProcessorValueTreeState::SliderAttachment inputGainAttachment  { apvts, "inputGain",  inputGain.slider };
    juce::AudioProcessorValueTreeState::SliderAttachment outputGainAttachment { apvts, "outputGain", outputGain.slider };

    juce::AudioProcessorValueTreeState::ButtonAttachment hpfEnabledAttachment   { apvts, "hpfEnabled", hpfEnabled.button };
    juce::AudioProcessorValueTreeState::ButtonAttachment compEnabledAttachment  { apvts, "compEnabled", compEnabled.button };
    juce::AudioProcessorValueTreeState::ButtonAttachment deessEnabledAttachment { apvts, "deessEnabled", deessEnabled.button };
    juce::AudioProcessorValueTreeState::ButtonAttachment masterBypassAttachment { apvts, "masterBypass", masterBypass.button };

    juce::AudioProcessorValueTreeState::SliderAttachment hpfFreqAttachment { apvts, "hpfFreq", hpfFreq.slider };

    juce::AudioProcessorValueTreeState::SliderAttachment eqLowFreqAttachment  { apvts, "eqLowFreq", eqLowFreq.slider };
    juce::AudioProcessorValueTreeState::SliderAttachment eqLowGainAttachment  { apvts, "eqLowGain", eqLowGain.slider };
    juce::AudioProcessorValueTreeState::SliderAttachment eqLowQAttachment     { apvts, "eqLowQ", eqLowQ.slider };
    juce::AudioProcessorValueTreeState::SliderAttachment eqMidFreqAttachment  { apvts, "eqMidFreq", eqMidFreq.slider };
    juce::AudioProcessorValueTreeState::SliderAttachment eqMidGainAttachment  { apvts, "eqMidGain", eqMidGain.slider };
    juce::AudioProcessorValueTreeState::SliderAttachment eqMidQAttachment     { apvts, "eqMidQ", eqMidQ.slider };
    juce::AudioProcessorValueTreeState::SliderAttachment eqHighFreqAttachment { apvts, "eqHighFreq", eqHighFreq.slider };
    juce::AudioProcessorValueTreeState::SliderAttachment eqHighGainAttachment { apvts, "eqHighGain", eqHighGain.slider };
    juce::AudioProcessorValueTreeState::SliderAttachment eqHighQAttachment    { apvts, "eqHighQ", eqHighQ.slider };

    juce::AudioProcessorValueTreeState::SliderAttachment compThresholdAttachment { apvts, "compThreshold", compThreshold.slider };
    juce::AudioProcessorValueTreeState::SliderAttachment compRatioAttachment     { apvts, "compRatio", compRatio.slider };
    juce::AudioProcessorValueTreeState::SliderAttachment compAttackAttachment    { apvts, "compAttack", compAttack.slider };
    juce::AudioProcessorValueTreeState::SliderAttachment compReleaseAttachment   { apvts, "compRelease", compRelease.slider };
    juce::AudioProcessorValueTreeState::SliderAttachment compMakeupAttachment    { apvts, "compMakeup", compMakeup.slider };

    juce::AudioProcessorValueTreeState::SliderAttachment deessFreqAttachment      { apvts, "deessFreq", deessFreq.slider };
    juce::AudioProcessorValueTreeState::SliderAttachment deessThresholdAttachment { apvts, "deessThreshold", deessThreshold.slider };
    juce::AudioProcessorValueTreeState::SliderAttachment deessAmountAttachment    { apvts, "deessAmount", deessAmount.slider };

    // --- chrome --------------------------------------------------------
    juce::Label titleLabel    { "title", "Berlin Vocal AI" };
    juce::Label phaseLabel    { "phase", "Phase 1 — Vocal Processor" };
    juce::TooltipWindow tooltipWindow { this, 400 };

    LevelMeterComponent inputMeter  { processorRef.getInputMeter(),  "IN" };
    LevelMeterComponent outputMeter { processorRef.getOutputMeter(), "OUT" };

    // Group box areas (computed in resized, painted in paint).
    juce::Rectangle<int> hpfBounds, eqBounds, compBounds, deessBounds;

    bool bypassed = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BerlinVocalAIAudioProcessorEditor)
};

} // namespace bva
