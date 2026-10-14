#include "PluginEditor.h"

namespace bva
{

//======================================================================
// LevelMeterComponent
//======================================================================
LevelMeterComponent::LevelMeterComponent (const LevelMeterSource& sourceToRead,
                                          const juce::String& name)
    : source (sourceToRead)
{
    setName (name);
    startTimerHz (30);
}

LevelMeterComponent::~LevelMeterComponent()
{
    stopTimer();
}

void LevelMeterComponent::timerCallback()
{
    // Smooth the visual fall a little further on the GUI side.
    const float target = source.getLevel();
    displayedLevel = juce::jmax (target, displayedLevel - 0.02f);

    if (std::abs (displayedLevel - target) > 0.0001f || target > 0.0f)
        repaint();
}

void LevelMeterComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (2.0f);

    g.setColour (juce::Colour (0xff14171b));
    g.fillRoundedRectangle (bounds, 3.0f);

    // Map RMS level to dBFS scale (-60..0 dB across the meter height).
    const float db = juce::Decibels::gainToDecibels (displayedLevel, -60.0f);
    const float norm = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);

    auto meterArea = bounds.reduced (3.0f);
    const float fillHeight = meterArea.getHeight() * norm;

    juce::ColourGradient gradient (
        juce::Colour (0xff4fd66a), meterArea.getX(), meterArea.getBottom(),
        juce::Colour (0xffd64f4f), meterArea.getX(), meterArea.getY(), false);
    gradient.addColour (0.70, juce::Colour (0xffd6c94f));

    g.setGradientFill (gradient);
    g.fillRoundedRectangle (meterArea.removeFromBottom (fillHeight), 2.0f);

    g.setColour (juce::Colour (0xff3a4048));
    g.drawRoundedRectangle (bounds, 3.0f, 1.0f);

    g.setColour (juce::Colour (0xff8b929c));
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (getName(), bounds, juce::Justification::centredBottom, false);
}

//======================================================================
// RotaryControl / ToggleControl
//======================================================================
BerlinVocalAIAudioProcessorEditor::RotaryControl::RotaryControl (
    const juce::String& labelText, const juce::String& tooltipText)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 0.75f,
                                juce::MathConstants<float>::pi * 2.25f, true);
    slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff4f8cff));
    slider.setColour (juce::Slider::thumbColourId, juce::Colour (0xffc9d3e3));
    slider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffaeb6c2));
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setTooltip (tooltipText);

    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    label.setColour (juce::Label::textColourId, juce::Colour (0xffaeb6c2));
}

BerlinVocalAIAudioProcessorEditor::ToggleControl::ToggleControl (const juce::String& labelText)
{
    button.setButtonText (labelText);
    button.setTooltip ("Enables or disables this module (click-free crossfade).");
    button.setColour (juce::ToggleButton::textColourId, juce::Colour (0xffaeb6c2));
    button.setColour (juce::ToggleButton::tickColourId, juce::Colour (0xff4f8cff));
    button.setColour (juce::ToggleButton::tickDisabledColourId, juce::Colour (0xff3a4048));
}

//======================================================================
// Editor
//======================================================================
BerlinVocalAIAudioProcessorEditor::BerlinVocalAIAudioProcessorEditor (
    BerlinVocalAIAudioProcessor& p)
    : AudioProcessorEditor (p), processorRef (p), apvts (p.getAPVTS())
{
    // Group headers and controls.
    titleLabel.setFont (juce::FontOptions (26.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xffe8eaed));
    titleLabel.setJustificationType (juce::Justification::centredLeft);

    phaseLabel.setFont (juce::FontOptions (13.0f));
    phaseLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8b929c));
    phaseLabel.setJustificationType (juce::Justification::centredLeft);

    masterBypass.button.setColour (juce::ToggleButton::textColourId, juce::Colour (0xffe8eaed));
    masterBypass.button.setTooltip ("Master bypass: crossfades the whole plugin back to the untouched input signal (50 ms).");
    masterBypass.button.onClick = [this]
    {
        bypassed = masterBypass.button.getToggleState();
        repaint();
    };

    // Dry/wet is the one horizontal slider (it is a blend, not a set-and-forget trim).
    dryWet.slider.setSliderStyle (juce::Slider::LinearHorizontal);
    dryWet.slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 20);
    dryWet.slider.setColour (juce::Slider::trackColourId, juce::Colour (0xff4f8cff));
    dryWet.slider.setColour (juce::Slider::thumbColourId, juce::Colour (0xffc9d3e3));

    for (auto* comp : std::initializer_list<juce::Component*>
    {
        &titleLabel, &phaseLabel, &inputMeter, &outputMeter,
        &inputGain.slider, &inputGain.label, &outputGain.slider, &outputGain.label,
        &dryWet.slider, &dryWet.label,
        &hpfEnabled.button, &hpfFreq.slider, &hpfFreq.label,
        &eqLowFreq.slider, &eqLowFreq.label, &eqLowGain.slider, &eqLowGain.label, &eqLowQ.slider, &eqLowQ.label,
        &eqMidFreq.slider, &eqMidFreq.label, &eqMidGain.slider, &eqMidGain.label, &eqMidQ.slider, &eqMidQ.label,
        &eqHighFreq.slider, &eqHighFreq.label, &eqHighGain.slider, &eqHighGain.label, &eqHighQ.slider, &eqHighQ.label,
        &compEnabled.button, &compThreshold.slider, &compThreshold.label,
        &compRatio.slider, &compRatio.label, &compAttack.slider, &compAttack.label,
        &compRelease.slider, &compRelease.label, &compMakeup.slider, &compMakeup.label,
        &deessEnabled.button, &deessFreq.slider, &deessFreq.label,
        &deessThreshold.slider, &deessThreshold.label, &deessAmount.slider, &deessAmount.label,
        &masterBypass.button
    })
        addAndMakeVisible (comp);

    setResizable (false, false);
    setSize (700, 900);
    startTimerHz (30);
}

BerlinVocalAIAudioProcessorEditor::~BerlinVocalAIAudioProcessorEditor()
{
    stopTimer();
}

void BerlinVocalAIAudioProcessorEditor::timerCallback()
{
    if (auto* raw = apvts.getRawParameterValue ("masterBypass"))
    {
        const bool b = raw->load (std::memory_order_relaxed) > 0.5f;
        if (b != bypassed)
        {
            bypassed = b;
            masterBypass.button.setToggleState (b, juce::dontSendNotification);
            repaint();
        }
    }
}

void BerlinVocalAIAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e2126));

    // Section group boxes (drawn behind the controls).
    auto groupBox = [] (juce::Graphics& g2, juce::Rectangle<int> area, const juce::String& title)
    {
        g2.setColour (juce::Colour (0xff262a31));
        g2.fillRoundedRectangle (area.toFloat(), 6.0f);
        g2.setColour (juce::Colour (0xff3a4048));
        g2.drawRoundedRectangle (area.toFloat(), 6.0f, 1.0f);
        g2.setColour (juce::Colour (0xff8b929c));
        g2.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g2.drawText (title, area.getX() + 10, area.getY() + 4, area.getWidth() - 20, 16,
                     juce::Justification::centredLeft, false);
    };

    groupBox (g, hpfBounds,   "HIGH-PASS FILTER (24 dB/oct)");
    groupBox (g, eqBounds,    "PARAMETRIC EQ");
    groupBox (g, compBounds,  "COMPRESSOR");
    groupBox (g, deessBounds, "DE-ESSER (simple split-band prototype)");

    if (bypassed)
    {
        g.setColour (juce::Colour (0x50101010));
        g.fillAll();
        g.setColour (juce::Colour (0xffd64f4f));
        g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
        g.drawText ("BYPASSED", getLocalBounds(), juce::Justification::centred, false);
    }
}

void BerlinVocalAIAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (12);

    // --- header ---------------------------------------------------------
    auto header = area.removeFromTop (52);
    auto bypassArea = header.removeFromRight (110);
    masterBypass.button.setBounds (bypassArea.removeFromTop (34));
    titleLabel.setBounds (header.removeFromLeft (380).removeFromTop (30));
    phaseLabel.setBounds (header.removeFromLeft (380).removeFromTop (18));
    area.reduce (0, 4);

    auto place = [] (juce::Component& c, juce::Rectangle<int> r, int size)
    {
        c.setBounds (r.removeFromLeft (size).withSizeKeepingCentre (size, size + 6));
    };

    // --- input row: gain + meter ---------------------------------------
    auto inputRow = area.removeFromTop (104);
    place (inputGain.slider, inputRow.withTrimmedLeft (10), 84);
    inputGain.label.setBounds (inputRow.getX() + 2, inputRow.getBottom() - 16, 84, 14);
    inputMeter.setBounds (inputRow.removeFromRight (34).withSizeKeepingCentre (26, 80));
    area.reduce (0, 6);

    // --- HPF group ------------------------------------------------------
    hpfBounds = area.removeFromTop (104);
    auto hpfInner = hpfBounds.reduced (10, 20);
    hpfEnabled.button.setBounds (hpfInner.removeFromLeft (70).withSizeKeepingCentre (64, 24));
    hpfInner.removeFromLeft (8);
    place (hpfFreq.slider, hpfInner.withTrimmedLeft (10), 72);
    hpfFreq.label.setBounds (hpfInner.getX() + 12, hpfBounds.getBottom() - 22, 72, 14);
    area.reduce (0, 6);

    // --- EQ group: three band rows --------------------------------------
    eqBounds = area.removeFromTop (228);
    {
        auto eqInner = eqBounds.reduced (10, 22);

        struct BandRow { RotaryControl* f; RotaryControl* g; RotaryControl* q; };
        const BandRow rows[] = { { &eqLowFreq, &eqLowGain, &eqLowQ },
                                 { &eqMidFreq, &eqMidGain, &eqMidQ },
                                 { &eqHighFreq, &eqHighGain, &eqHighQ } };

        for (const auto& row : rows)
        {
            auto r = eqInner.removeFromTop (66);
            auto cell = [r] (int index) { return r.getX() + 16 + index * 130; };

            row.f->slider.setBounds (juce::Rectangle<int> (cell (0), r.getY(), 64, 60));
            row.f->label.setBounds  (juce::Rectangle<int> (cell (0), r.getBottom() - 12, 64, 12));
            row.g->slider.setBounds (juce::Rectangle<int> (cell (1), r.getY(), 64, 60));
            row.g->label.setBounds  (juce::Rectangle<int> (cell (1), r.getBottom() - 12, 64, 12));
            row.q->slider.setBounds (juce::Rectangle<int> (cell (2), r.getY(), 64, 60));
            row.q->label.setBounds  (juce::Rectangle<int> (cell (2), r.getBottom() - 12, 64, 12));
        }
    }
    area.reduce (0, 6);

    // --- Compressor group ------------------------------------------------
    compBounds = area.removeFromTop (170);
    {
        auto cInner = compBounds.reduced (10, 20);
        compEnabled.button.setBounds (cInner.removeFromLeft (70).withSizeKeepingCentre (64, 24));
        cInner.removeFromLeft (6);

        RotaryControl* compSliders[] = { &compThreshold, &compRatio, &compAttack, &compRelease, &compMakeup };
        const int step = (cInner.getWidth() - 72) / 5;
        int x = cInner.getX() + 4;
        for (auto* rc : compSliders)
        {
            rc->slider.setBounds (x, cInner.getY(), 68, 62);
            rc->label.setBounds  (x, cInner.getBottom() - 14, 68, 12);
            x += step;
        }
    }
    area.reduce (0, 6);

    // --- De-esser group ---------------------------------------------------
    deessBounds = area.removeFromTop (140);
    {
        auto dInner = deessBounds.reduced (10, 20);
        deessEnabled.button.setBounds (dInner.removeFromLeft (70).withSizeKeepingCentre (64, 24));
        dInner.removeFromLeft (6);

        RotaryControl* deessSliders[] = { &deessFreq, &deessThreshold, &deessAmount };
        const int step = (dInner.getWidth() - 60) / 3;
        int x = dInner.getX() + 4;
        for (auto* rc : deessSliders)
        {
            rc->slider.setBounds (x, dInner.getY(), 68, 62);
            rc->label.setBounds  (x, dInner.getBottom() - 14, 68, 12);
            x += step;
        }
    }
    area.reduce (0, 6);

    // --- output row: gain + meter + dry/wet ------------------------------
    auto outputRow = area.removeFromTop (104);
    place (outputGain.slider, outputRow.withTrimmedLeft (10), 84);
    outputGain.label.setBounds (outputRow.getX() + 2, outputRow.getBottom() - 16, 84, 14);
    outputMeter.setBounds (outputRow.removeFromRight (34).withSizeKeepingCentre (26, 80));

    auto dwArea = outputRow.withSizeKeepingCentre (outputRow.getWidth() - 20, 46);
    dryWet.label.setBounds (dwArea.removeFromLeft (64));
    dryWet.slider.setBounds (dwArea.reduced (4, 8));
}

} // namespace bva
