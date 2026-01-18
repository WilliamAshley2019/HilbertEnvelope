// HilbertEnvelopeEditor.cpp - UPDATED WITH VERTICAL METERS AND SCOPE
#include "HilbertEnvelopeEditor.h"

//==============================================================================
// ParameterKnobWithDisplays Implementation
//==============================================================================
ParameterKnobWithDisplays::ParameterKnobWithDisplays()
{
    // Configure knob
    knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    knob.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
        juce::MathConstants<float>::pi * 2.75f, true);
    knob.setLookAndFeel(&blackMetalLNF);
    knob.setVelocityBasedMode(false);
    knob.setRange(0.0, 1.0, 0.01);
    knob.setValue(0.5);
    knob.setDoubleClickReturnValue(true, 0.5);
    knob.onValueChange = [this] { updateValueLabel(); };

    // Configure label
    label.setText("PARAM", juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    label.setColour(juce::Label::textColourId, juce::Colours::white);

    // Configure value label
    valueLabel.setText("0.00", juce::dontSendNotification);
    valueLabel.setJustificationType(juce::Justification::centred);
    valueLabel.setFont(juce::FontOptions(11.0f, juce::Font::plain));
    valueLabel.setColour(juce::Label::textColourId, juce::Colour(180, 180, 180));

    // Configure block display
    blockDisplay.setSize(100, 15);
    blockDisplay.setValue(0.5f);

    addAndMakeVisible(knob);
    addAndMakeVisible(label);
    addAndMakeVisible(valueLabel);
    addAndMakeVisible(blockDisplay);
}

ParameterKnobWithDisplays::~ParameterKnobWithDisplays()
{
    knob.setLookAndFeel(nullptr);
}

void ParameterKnobWithDisplays::attachParameter(juce::AudioProcessorValueTreeState& apvts,
    const juce::String& paramID,
    const juce::String& lbl,
    const juce::String& unit)
{
    parameter = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(paramID));
    paramLabel = lbl;
    paramUnit = unit;

    if (parameter != nullptr)
    {
        knob.setRange(parameter->range.start,
            parameter->range.end,
            parameter->range.interval);
        knob.setValue(parameter->get(), juce::dontSendNotification);
    }

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, paramID, knob);

    label.setText(lbl, juce::dontSendNotification);
    updateValueLabel();
}

void ParameterKnobWithDisplays::resized()
{
    auto area = getLocalBounds();

    // Label at top
    label.setBounds(area.removeFromTop(20));

    // Knob in the middle
    auto knobArea = area.removeFromTop(80).reduced(10);
    int knobSize = juce::jmin(knobArea.getWidth(), knobArea.getHeight());
    knob.setBounds(knobArea.withSizeKeepingCentre(knobSize, knobSize));

    // Value label below knob
    valueLabel.setBounds(area.removeFromTop(20));

    // Block display at the bottom
    blockDisplay.setBounds(area.reduced(5, 5));
}

void ParameterKnobWithDisplays::updateDisplay()
{
    updateValueLabel();
}

void ParameterKnobWithDisplays::setBlockValue(float value)
{
    blockDisplay.setValue(value);

    // Also update the block display text
    juce::String text;
    if (value < 0.01f) text = "0";
    else if (value > 0.99f) text = "MAX";
    else text = juce::String(value * 100.0f, 0) + "%";

    blockDisplay.setValueText(text);
}

void ParameterKnobWithDisplays::updateValueLabel()
{
    if (!parameter) return;

    const float value = knob.getValue();
    juce::String display;

    if (paramUnit == "s") display = juce::String(value, 1) + "s";
    else if (paramUnit == "ms") display = juce::String(value, 0) + "ms";
    else if (paramUnit == "%")
    {
        display = juce::String(value * 100.0f, 0) + "%";
        // Also update block display for percentage parameters
        setBlockValue(value);
    }
    else if (paramUnit == "x") display = juce::String(value, 2) + "x";
    else if (paramUnit == "db")
    {
        float dbValue = 20.0f * static_cast<float>(std::log10(value + 0.0001f));
        display = juce::String(dbValue, 1) + " dB";
    }
    else
    {
        display = juce::String(value, 2);
        // For non-percentage, show normalized value in block
        float normalizedValue = (value - parameter->range.start) /
            (parameter->range.end - parameter->range.start);
        setBlockValue(normalizedValue);
    }

    valueLabel.setText(display, juce::dontSendNotification);
}

//==============================================================================
// EnvelopeMeterWithBlock Implementation (kept for compatibility, but we'll use vertical meters)
//==============================================================================
EnvelopeMeterWithBlock::EnvelopeMeterWithBlock()
{
    setSize(250, 100);
    blockDisplay.setSize(230, 20);
    addAndMakeVisible(blockDisplay);
}

void EnvelopeMeterWithBlock::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(30, 32, 35));
    g.setColour(juce::Colour(60, 63, 70));
    g.drawRect(getLocalBounds(), 2);

    // Label
    g.setColour(juce::Colour(220, 220, 220));
    g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    g.drawText(labelText, 0, 10, getWidth(), 20, juce::Justification::centred);

    // Value display
    g.setFont(juce::FontOptions(12.0f, juce::Font::plain));

    juce::String valueStr;
    if (currentValue < 0.0001f)
    {
        valueStr = "0.0000 (-∞ dB)";
    }
    else
    {
        valueStr = juce::String(currentValue, 4);
        float dbValue = 20.0f * static_cast<float>(std::log10(currentValue));
        valueStr += " (" + juce::String(dbValue, 1) + " dB)";
    }

    g.drawText(valueStr, 0, 35, getWidth(), 20, juce::Justification::centred);

    // Peak hold indicator (if this is the peak meter)
    if (labelText.contains("PEAK"))
    {
        g.setColour(juce::Colour(255, 100, 100).withAlpha(0.3f));
        g.drawLine(0.0f, 30.0f, static_cast<float>(getWidth()), 30.0f, 1.0f);

        g.setFont(juce::FontOptions(10.0f, juce::Font::italic));
        g.drawText("PEAK HOLD", 0, 85, getWidth(), 15, juce::Justification::centred);
    }
}

void EnvelopeMeterWithBlock::setValue(float value)
{
    currentValue = juce::jlimit(0.0f, 1.0f, value);
    blockDisplay.setValue(currentValue);

    // Format text for block display
    juce::String blockText;
    if (currentValue < 0.001f) blockText = "SILENT";
    else if (currentValue > 0.99f) blockText = "CLIPPING!";
    else blockText = juce::String(currentValue * 100.0f, 1) + "%";

    blockDisplay.setValueText(blockText);
    repaint();
}

void EnvelopeMeterWithBlock::setLabel(const juce::String& text)
{
    if (labelText != text)
    {
        labelText = text;
        repaint();
    }
}

void EnvelopeMeterWithBlock::setBlockValue(float value)
{
    blockValue = value;
    blockDisplay.setValue(value);

    juce::String text = juce::String(value * 100.0f, 1) + "%";
    blockDisplay.setValueText(text);
}

//==============================================================================
// VerticalEnvelopeMeter Implementation (NEW)
//==============================================================================
VerticalEnvelopeMeter::VerticalEnvelopeMeter()
{
    setSize(60, 200);
}

void VerticalEnvelopeMeter::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(25, 27, 30));
    g.setColour(juce::Colour(40, 42, 46));
    g.drawRect(getLocalBounds(), 2);

    // Draw meter scale
    g.setColour(juce::Colour(80, 82, 86));
    for (int i = 0; i <= 10; ++i)
    {
        float y = getHeight() * i / 10.0f;
        g.drawHorizontalLine(static_cast<int>(y), 10, getWidth() - 10);
    }

    // Draw dB labels
    g.setColour(juce::Colour(150, 150, 150));
    g.setFont(juce::FontOptions(9.0f, juce::Font::plain));
    g.drawText("0", 5, 0, 20, 15, juce::Justification::centred);
    g.drawText("-10", 5, getHeight() * 0.25f - 7, 20, 15, juce::Justification::centred);
    g.drawText("-20", 5, getHeight() * 0.5f - 7, 20, 15, juce::Justification::centred);
    g.drawText("-40", 5, getHeight() * 0.75f - 7, 20, 15, juce::Justification::centred);
    g.drawText("-∞", 5, getHeight() - 15, 20, 15, juce::Justification::centred);

    // Draw meter value - convert linear to logarithmic for visual
    float logValue = 0.0f;
    if (value > 0.0001f)
    {
        // Convert to dB scale for display: 0dB at top, -∞ at bottom
        logValue = 1.0f - (std::log10(value * 9.0f + 1.0f) / std::log10(10.0f));
    }
    else
    {
        logValue = 1.0f; // All the way at bottom for silence
    }

    float meterHeight = getHeight() * logValue;
    meterHeight = juce::jlimit(0.0f, static_cast<float>(getHeight()), meterHeight);

    // Gradient fill for meter (green at bottom, red at top)
    juce::ColourGradient meterGrad(
        juce::Colour(100, 255, 100), 0, getHeight(),  // Green at bottom (silence)
        juce::Colour(255, 100, 100), 0, 0,            // Red at top (clipping)
        false);

    // Fill the portion from meterHeight to bottom
    g.setGradientFill(meterGrad);

    // FIXED LINE 265: Use all float arguments
    g.fillRect(25.0f, meterHeight, static_cast<float>(getWidth() - 40), getHeight() - meterHeight);

    // Draw current value line
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.drawHorizontalLine(static_cast<int>(meterHeight), 25, getWidth() - 15);

    // Draw current value indicator (already uses integers - OK)
    g.setColour(juce::Colours::white);
    g.fillRect(getWidth() - 15, static_cast<int>(meterHeight) - 1, 10, 3);

    // Draw value text
    g.setColour(juce::Colour(200, 200, 200));
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    juce::String valueStr;
    if (value < 0.001f)
        valueStr = "-∞ dB";
    else
        valueStr = juce::String(20.0f * std::log10(value), 1) + " dB";
    g.drawText(valueStr, 0, getHeight() - 25, getWidth(), 20, juce::Justification::centred);
}

void VerticalEnvelopeMeter::setValue(float newValue)
{
    value = juce::jlimit(0.0f, 1.0f, newValue);
    repaint();
}

//==============================================================================
// Main Editor Implementation
//==============================================================================
HilbertEnvelopeEditor::HilbertEnvelopeEditor(HilbertEnvelopeProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(1000, 700);  // Increased size for scope
    setResizable(true, true);
    setResizeLimits(800, 550, 1400, 900);

    // Setup knobs with block displays
    auto& apvts = processor.getValueTreeState();
    mixKnob.attachParameter(apvts, "mix", "MIX", "%");
    gainKnob.attachParameter(apvts, "gain", "GAIN", "x");
    attackKnob.attachParameter(apvts, "attack", "ATTACK", "ms");
    releaseKnob.attachParameter(apvts, "release", "RELEASE", "ms");

    addAndMakeVisible(mixKnob);
    addAndMakeVisible(gainKnob);
    addAndMakeVisible(attackKnob);
    addAndMakeVisible(releaseKnob);

    // Setup vertical meters labels
    meterLabelCurrent.setText("CURRENT ENVELOPE", juce::dontSendNotification);
    meterLabelCurrent.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    meterLabelCurrent.setColour(juce::Label::textColourId, juce::Colour(100, 200, 255));
    meterLabelCurrent.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(meterLabelCurrent);

    meterLabelPeak.setText("PEAK HOLD", juce::dontSendNotification);
    meterLabelPeak.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    meterLabelPeak.setColour(juce::Label::textColourId, juce::Colour(255, 100, 100));
    meterLabelPeak.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(meterLabelPeak);

    // Setup vertical meters
    addAndMakeVisible(currentEnvelopeMeter);
    addAndMakeVisible(peakEnvelopeMeter);

    // Setup scope
    scopeLabel.setText("ENVELOPE SCOPE", juce::dontSendNotification);
    scopeLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    scopeLabel.setColour(juce::Label::textColourId, juce::Colour(180, 180, 180));
    scopeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(scopeLabel);

    addAndMakeVisible(envelopeScope);

    // Setup mode selector
    modeLabel.setText("PROCESSING MODE:", juce::dontSendNotification);
    modeLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    modeLabel.setColour(juce::Label::textColourId, juce::Colour(200, 200, 200));
    modeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(modeLabel);

    // Configure mode selector
    modeSelector.addItem("Instant Envelope", 1);
    modeSelector.addItem("Smoothed (A/R)", 2);
    modeSelector.addItem("Sidechain", 3);
    modeSelector.setSelectedId(1, juce::dontSendNotification);

    // Style the combobox
    modeSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colour(40, 40, 45));
    modeSelector.setColour(juce::ComboBox::textColourId, juce::Colour(220, 220, 220));
    modeSelector.setColour(juce::ComboBox::arrowColourId, juce::Colour(180, 180, 180));
    modeSelector.setColour(juce::ComboBox::outlineColourId, juce::Colour(80, 80, 85));

    // Attach to parameter
    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, "mode", modeSelector);

    addAndMakeVisible(modeSelector);

    // Setup labels
    titleLabel.setText("HILBERT ENVELOPE DETECTOR", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(26.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(220, 220, 220));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    infoLabel.setText("Analytic Signal: |x(t) + j·H{x(t)}|", juce::dontSendNotification);
    infoLabel.setFont(juce::FontOptions(14.0f, juce::Font::plain));
    infoLabel.setColour(juce::Label::textColourId, juce::Colour(180, 180, 180));
    infoLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(infoLabel);

    statusLabel.setText("Ready", juce::dontSendNotification);
    statusLabel.setFont(juce::FontOptions(11.0f, juce::Font::plain));
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(150, 200, 150));
    statusLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(statusLabel);

    // Start timer for updates
    startTimerHz(30);
}

HilbertEnvelopeEditor::~HilbertEnvelopeEditor() {}

//==============================================================================
void HilbertEnvelopeEditor::paint(juce::Graphics& g)
{
    // Dark metal background with subtle gradient
    juce::ColourGradient bgGrad(
        juce::Colour(28, 30, 34), 0.0f, 0.0f,
        juce::Colour(22, 24, 28), 0.0f, static_cast<float>(getHeight()), false);
    g.setGradientFill(bgGrad);
    g.fillAll();

    // Top panel with embossed effect
    g.setColour(juce::Colour(40, 42, 46));
    g.fillRect(0, 0, getWidth(), 140);

    // Subtle panel separators
    g.setColour(juce::Colour(50, 52, 58));
    g.drawLine(0.0f, 140.0f, static_cast<float>(getWidth()), 140.0f, 2.0f);

    // Draw meter area separator
    float meterAreaStart = getHeight() - 400;  // Adjust based on your layout
    g.drawLine(0.0f, meterAreaStart, static_cast<float>(getWidth()), meterAreaStart, 1.0f);

    // Decorative corner elements
    g.setColour(juce::Colour(80, 85, 90));
    float cornerSize = 15.0f;
    g.drawLine(0.0f, 0.0f, cornerSize, 0.0f, 2.0f);
    g.drawLine(0.0f, 0.0f, 0.0f, cornerSize, 2.0f);
    g.drawLine(static_cast<float>(getWidth()), 0.0f,
        static_cast<float>(getWidth()) - cornerSize, 0.0f, 2.0f);
    g.drawLine(static_cast<float>(getWidth()), 0.0f,
        static_cast<float>(getWidth()), cornerSize, 2.0f);

    // Footer text
    g.setColour(juce::Colour(120, 125, 130));
    g.setFont(juce::FontOptions(10.0f, juce::Font::plain));
    g.drawText("21-tap FIR Hilbert Transform | Phase-Independent Amplitude Detection",
        0, getHeight() - 25, getWidth(), 20, juce::Justification::centred);

    // Draw subtle grid pattern in background
    g.setColour(juce::Colour(35, 37, 40).withAlpha(0.1f));
    for (int i = 0; i < getWidth(); i += 20)
    {
        g.drawVerticalLine(i, 0, getHeight());
    }
    for (int i = 0; i < getHeight(); i += 20)
    {
        g.drawHorizontalLine(i, 0, getWidth());
    }
}

//==============================================================================
void HilbertEnvelopeEditor::resized()
{
    auto area = getLocalBounds();

    // Title area (top 100px)
    auto titleArea = area.removeFromTop(100);
    titleLabel.setBounds(titleArea.reduced(20, 15));
    infoLabel.setBounds(titleArea.reduced(40, 30).translated(0, 30));
    statusLabel.setBounds(titleArea.reduced(40, 10).translated(0, 60));

    // Mode selector area (below title)
    auto modeArea = area.removeFromTop(40);
    modeLabel.setBounds(modeArea.removeFromLeft(150).reduced(5));
    modeSelector.setBounds(modeArea.reduced(5));

    // Knob area (next 150px)
    auto knobArea = area.removeFromTop(150);
    const int knobWidth = knobArea.getWidth() / 4;

    mixKnob.setBounds(knobArea.removeFromLeft(knobWidth).reduced(10, 5));
    gainKnob.setBounds(knobArea.removeFromLeft(knobWidth).reduced(10, 5));
    attackKnob.setBounds(knobArea.removeFromLeft(knobWidth).reduced(10, 5));
    releaseKnob.setBounds(knobArea.removeFromLeft(knobWidth).reduced(10, 5));

    // Bottom area: meters and scope (rest of the space)
    auto bottomArea = area;

    // Left: Vertical meters (200px wide)
    auto meterArea = bottomArea.removeFromLeft(200).reduced(10, 15);

    // Current envelope meter
    meterLabelCurrent.setBounds(meterArea.removeFromTop(20));
    currentEnvelopeMeter.setBounds(meterArea.removeFromTop(meterArea.getHeight() * 0.45).reduced(5, 10));

    // Add some spacing between meters
    meterArea.removeFromTop(15);

    // Peak envelope meter
    meterLabelPeak.setBounds(meterArea.removeFromTop(20));
    peakEnvelopeMeter.setBounds(meterArea.reduced(5, 10));

    // Right: Scope (takes remaining space)
    auto scopeArea = bottomArea.reduced(10, 15);
    scopeLabel.setBounds(scopeArea.removeFromTop(20));
    envelopeScope.setBounds(scopeArea);
}

//==============================================================================
void HilbertEnvelopeEditor::timerCallback()
{
    updateDisplays();
}

void HilbertEnvelopeEditor::updateDisplays()
{
    try
    {
        // Get envelope values
        float currentEnv = processor.getCurrentEnvelope();
        float peakEnv = processor.getPeakEnvelope();

        // Update vertical meters
        currentEnvelopeMeter.setValue(currentEnv);
        peakEnvelopeMeter.setValue(peakEnv);

        // Update scope with envelope values
        envelopeScope.pushSample(currentEnv, peakEnv);

        // Get parameter values
        auto& apvts = processor.getValueTreeState();

        // Update each knob's block display with ITS OWN value
        // MIX knob: show mix percentage (0-100%)
        float mixValue = apvts.getRawParameterValue("mix")->load();
        mixKnob.setBlockValue(mixValue);  // mixValue is already 0-1

        // GAIN knob: show gain level (0-4x)
        float gainValue = apvts.getRawParameterValue("gain")->load();
        gainKnob.setBlockValue(gainValue / 4.0f);  // Normalize 0-4 to 0-1

        // ATTACK knob: show attack time (1-500ms)
        float attackValue = apvts.getRawParameterValue("attack")->load();
        attackKnob.setBlockValue(attackValue / 500.0f);  // Normalize

        // RELEASE knob: show release time (1-2000ms)
        float releaseValue = apvts.getRawParameterValue("release")->load();
        releaseKnob.setBlockValue(releaseValue / 2000.0f);  // Normalize

        // Update status based on envelope level
        juce::String status;
        if (currentEnv > 0.95f)
            status = "NEAR CLIPPING!";
        else if (currentEnv > 0.7f)
            status = "HOT SIGNAL";
        else if (currentEnv > 0.3f)
            status = "GOOD LEVEL";
        else if (currentEnv > 0.01f)
            status = "SIGNAL PRESENT";
        else
            status = "NO SIGNAL";

        // Add mode info to status
        int mode = static_cast<int>(apvts.getRawParameterValue("mode")->load());
        juce::String modeStr;
        switch (mode)
        {
        case 0: modeStr = " | INSTANT"; break;
        case 1: modeStr = " | SMOOTHED"; break;
        case 2: modeStr = " | SIDECHAIN"; break;
        default: modeStr = "";
        }

        // Add peak level info
        juce::String peakStr;
        if (peakEnv > 0.001f)
        {
            float peakDb = 20.0f * std::log10(peakEnv);
            peakStr = " | PEAK: " + juce::String(peakDb, 1) + " dB";
        }

        statusLabel.setText(status + modeStr + peakStr, juce::dontSendNotification);
    }
    catch (const std::exception& e)
    {
        statusLabel.setText("Error: " + juce::String(e.what()), juce::dontSendNotification);
    }
    catch (...)
    {
        statusLabel.setText("UPDATING...", juce::dontSendNotification);
    }
}