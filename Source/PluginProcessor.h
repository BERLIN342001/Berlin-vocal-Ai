#pragma once
/*
    PluginProcessor.h
    -----------------
    Host-facing AudioProcessor for Berlin Vocal AI (Phase 1).

    Responsibilities:
    - Own the APVTS (single source of truth for parameters).
    - Forward validated parameter snapshots to the DSP engine once per
      block (audio thread).
    - Own the VocalProcessor (all DSP state lives there).
    - Save/restore the full parameter state via the host mechanism.

    GUI code lives in PluginEditor; it never touches the DSP engine
    directly, only parameters and meter atomics.
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "dsp/VocalProcessor.h"

class BerlinVocalAIAudioProcessor : public juce::AudioProcessor
{
public:
    BerlinVocalAIAudioProcessor();

    //== AudioProcessor ==================================================
    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //== Editor ==========================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //== State ===========================================================
    const juce::String getName() const override
    {
       #ifdef JucePlugin_Name
        return JucePlugin_Name;
       #else
        // Headless test builds do not define plugin macros.
        return "Berlin Vocal AI";
       #endif
    }
    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override    { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //== Public for editor/tests =========================================
    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

    // Meter sources for the editor (atomic reads only).
    const bva::LevelMeterSource& getInputMeter()  const noexcept { return vocalProcessor.inputMeter; }
    const bva::LevelMeterSource& getOutputMeter() const noexcept { return vocalProcessor.outputMeter; }

private:
    juce::AudioProcessorValueTreeState apvts;

    bva::VocalProcessor vocalProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BerlinVocalAIAudioProcessor)
};
