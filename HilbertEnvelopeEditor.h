// HilbertEnvelopeEditor.h - UPDATED VERSION
#pragma once

#include <JuceHeader.h>
#include "HilbertEnvelopeProcessor.h"
#include "ThinBlockLcdDisplay.h"
#include "BlackMetalKnobLNF.h"
#include "BlackMetalSliderLNF.h"
#include "BlackMetalVerticalSliderLNF.h"
#include "EnvelopeScope.h"

//==============================================================================
// Parameter Knob with LCD AND Block Display
//==============================================================================
class ParameterKnobWithDisplays : public juce::Component
{
public:
    ParameterKnobWithDisplays();
    ~ParameterKnobWithDisplays() override;

    void attachParameter(juce::AudioProcessorValueTreeState& apvts,
        const juce::String& paramID,
        const juce::String& lbl,
        const juce::String& unit);

    void resized() override;
    void updateDisplay();
    void setBlockValue(float value);

private:
    juce::Slider knob;
    juce::Label label;
    juce::Label valueLabel;
    ThinBlockLcdDisplay blockDisplay;
    BlackMetalKnobLNF blackMetalLNF;

    juce::AudioParameterFloat* parameter = nullptr;
    juce::String paramLabel, paramUnit;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    void updateValueLabel();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterKnobWithDisplays)
};

//==============================================================================
// Envelope Meter with Block Display (KEPT FOR COMPATIBILITY BUT NOT USED)
//==============================================================================
class EnvelopeMeterWithBlock : public juce::Component
{
public:
    EnvelopeMeterWithBlock();
    void paint(juce::Graphics& g) override;
    void setValue(float value);
    void setLabel(const juce::String& text);
    void setBlockValue(float value);

private:
    float currentValue = 0.0f;
    float blockValue = 0.0f;
    juce::String labelText;
    ThinBlockLcdDisplay blockDisplay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeMeterWithBlock)
};

//==============================================================================
// Vertical Envelope Meter (NEW - REPLACES HORIZONTAL METERS)
//==============================================================================
class VerticalEnvelopeMeter : public juce::Component
{
public:
    VerticalEnvelopeMeter();
    void paint(juce::Graphics& g) override;
    void setValue(float newValue);

private:
    float value = 0.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VerticalEnvelopeMeter)
};

//==============================================================================
// Main Plugin Editor (UPDATED)
//==============================================================================
class HilbertEnvelopeEditor : public juce::AudioProcessorEditor,
    private juce::Timer
{
public:
    explicit HilbertEnvelopeEditor(HilbertEnvelopeProcessor&);
    ~HilbertEnvelopeEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    HilbertEnvelopeProcessor& processor;

    // GUI Components
    ParameterKnobWithDisplays mixKnob;
    ParameterKnobWithDisplays gainKnob;
    ParameterKnobWithDisplays attackKnob;
    ParameterKnobWithDisplays releaseKnob;

    // NEW: Vertical meters instead of horizontal EnvelopeMeterWithBlock
    VerticalEnvelopeMeter currentEnvelopeMeter;
    VerticalEnvelopeMeter peakEnvelopeMeter;

    // NEW: Scope visualization
    EnvelopeScope envelopeScope;

    // Labels
    juce::Label titleLabel;
    juce::Label infoLabel;
    juce::Label modeLabel;
    juce::Label statusLabel;
    juce::Label meterLabelCurrent;
    juce::Label meterLabelPeak;
    juce::Label scopeLabel;

    // Mode selector
    juce::ComboBox modeSelector;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;

    void timerCallback() override;
    void updateDisplays();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HilbertEnvelopeEditor)
};