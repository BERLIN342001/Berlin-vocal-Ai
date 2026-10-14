#include "PluginProcessor.h"
#include "PluginEditor.h"

BerlinVocalAIAudioProcessor::BerlinVocalAIAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "BERLIN_VOCAL_AI_PARAMS", bva::createParameterLayout())
{
    // NOTE: Parameter version hints are not exposed by the JUCE version
    // pinned for Phase 1 (no public setter on AudioProcessorParameter),
    // so none are set. If parameter meanings ever change, migrate via
    // NEW parameter IDs and a state-format version field instead.
}

void BerlinVocalAIAudioProcessor::prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock)
{
    vocalProcessor.prepare (sampleRate, (int) maximumExpectedSamplesPerBlock);
}

void BerlinVocalAIAudioProcessor::releaseResources()
{
    // Nothing to release: all DSP state is value-owned and reset instead.
    vocalProcessor.reset();
}

bool BerlinVocalAIAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();

    if (in.isDisabled() || out.isDisabled())
        return false;

    // A vocal effect: identical mono->mono or stereo->stereo configs.
    // (Mixed mono->stereo is intentionally not offered in Phase 1.)
    if (in != out)
        return false;

    return in == juce::AudioChannelSet::mono()
        || in == juce::AudioChannelSet::stereo();
}

void BerlinVocalAIAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Defensive: silence any channels beyond what the DSP engine handles.
    const int numMainOut = getMainBusNumOutputChannels();
    for (int ch = bva::VocalProcessor::maxChannels; ch < numMainOut; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    // Snapshot parameters (atomic reads) and push into the engine once
    // per block. This is the ONLY parameter path into the audio thread.
    vocalProcessor.setParameterValues (bva::snapshotValues (apvts));
    vocalProcessor.process (buffer);
}

void BerlinVocalAIAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Full parameter state as XML. Stable parameter IDs make this
    // forward/backward compatible within Phase 1.
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void BerlinVocalAIAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            // replaceState is the supported host-state entry point; it
            // synchronises parameters and notifies listeners/attachments.
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
        }
    }
}

juce::AudioProcessorEditor* BerlinVocalAIAudioProcessor::createEditor()
{
    return new bva::BerlinVocalAIAudioProcessorEditor (*this);
}

// This creates the plugin instances used by the VST3 wrapper at build time.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BerlinVocalAIAudioProcessor();
}
