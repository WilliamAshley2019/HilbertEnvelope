// HilbertEnvelopeProcessor.cpp - UPDATED WITH FIXED MATH AND BETTER MIXING
#include "HilbertEnvelopeProcessor.h"
#include "HilbertEnvelopeEditor.h"

HilbertEnvelopeProcessor::HilbertEnvelopeProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    parameters(*this, nullptr, "HilbertParams", {
      std::make_unique<juce::AudioParameterFloat>("mix", "Mix",
          juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.25f),  // Lower default
      std::make_unique<juce::AudioParameterFloat>("gain", "Gain",
          juce::NormalisableRange<float>(0.0f, 4.0f, 0.01f), 1.0f),
      std::make_unique<juce::AudioParameterFloat>("attack", "Attack",
          juce::NormalisableRange<float>(1.0f, 500.0f, 0.1f), 10.0f),
      std::make_unique<juce::AudioParameterFloat>("release", "Release",
          juce::NormalisableRange<float>(1.0f, 2000.0f, 0.1f), 100.0f),
      std::make_unique<juce::AudioParameterChoice>("mode", "Mode",
          juce::StringArray{"Instant", "Smoothed", "Sidechain"}, 0)
        })
{
    mixParam = parameters.getRawParameterValue("mix");
    gainParam = parameters.getRawParameterValue("gain");
    attackParam = parameters.getRawParameterValue("attack");
    releaseParam = parameters.getRawParameterValue("release");
    modeParam = parameters.getRawParameterValue("mode");

    initializeHilbertFilter();
}

HilbertEnvelopeProcessor::~HilbertEnvelopeProcessor() {}

void HilbertEnvelopeProcessor::initializeHilbertFilter()
{
    // FIXED: 31-tap Hilbert transformer (removed trailing zeros)
    hilbertCoeffs = {
        -0.0164f, 0.0f, -0.0265f, 0.0f, -0.0452f, 0.0f, -0.0909f, 0.0f,
        -0.3129f, 0.0f, -0.5000f, 0.0f, -0.3129f, 0.0f, -0.0909f, 0.0f,
        -0.0452f, 0.0f, -0.0265f, 0.0f, -0.0164f  // REMOVED: , -0.0103f, 0.0f, -0.0060f, 0.0f, -0.0030f, 0.0f, -0.0012f, 0.0f, -0.0003f, 0.0f
    };
    filterTaps = static_cast<int>(hilbertCoeffs.size());  // Now 21
}

float HilbertEnvelopeProcessor::processEnvelopeSmoothing(float input, float currentState,
    float attackCoeff, float releaseCoeff)
{
    if (input > currentState)
    {
        // Rising: use attack coefficient
        return attackCoeff * currentState + (1.0f - attackCoeff) * input;
    }
    else
    {
        // Falling: use release coefficient
        return releaseCoeff * currentState + (1.0f - releaseCoeff) * input;
    }
}

float HilbertEnvelopeProcessor::createOutput(float input, float envelope, float mix, float gain)
{
    // FIXED: Use modulation approach instead of additive mixing
    // envelope is 0-1, so when mix=1, output = input * envelope
    // when mix=0, output = input * 1.0 (dry)
    float modulationFactor = (1.0f - mix) + mix * envelope;
    float modulated = input * modulationFactor;

    // Softer clipping with tanh
    return std::tanh(modulated * gain * 0.5f);
}

void HilbertEnvelopeProcessor::pushScopeSample(float env, float peak)
{
    scopeCurrentEnvelope.store(env);
    scopePeakEnvelope.store(peak);
}

void HilbertEnvelopeProcessor::updateSmoothingCoefficients()
{
    // Convert milliseconds to seconds
    const float attackTimeS = attackParam->load() * 0.001f;
    const float releaseTimeS = releaseParam->load() * 0.001f;

    // Calculate target coefficients using exponential formula
    targetAttackCoeff = std::exp(-1.0f / (attackTimeS * static_cast<float>(sampleRate)));
    targetReleaseCoeff = std::exp(-1.0f / (releaseTimeS * static_cast<float>(sampleRate)));

    // For very short times, ensure coefficients don't go to 0
    targetAttackCoeff = juce::jlimit(0.0001f, 0.9999f, targetAttackCoeff);
    targetReleaseCoeff = juce::jlimit(0.0001f, 0.9999f, targetReleaseCoeff);
}

void HilbertEnvelopeProcessor::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    hilbertBuffer.assign(filterTaps, 0.0f);
    bufferIndex = 0;
    currentEnvelope = 0.0f;
    peakEnvelope = 0.0f;
    scopeCurrentEnvelope = 0.0f;
    scopePeakEnvelope = 0.0f;

    // Initialize smoothing coefficients
    updateSmoothingCoefficients();
    currentAttackCoeff = targetAttackCoeff;
    currentReleaseCoeff = targetReleaseCoeff;

    // Initialize channel states
    channelStates.clear();
}

void HilbertEnvelopeProcessor::releaseResources() {}

void HilbertEnvelopeProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that don't contain input data
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    const int numSamples = buffer.getNumSamples();
    const float mix = mixParam->load();
    const float gain = gainParam->load();
    const int mode = static_cast<int>(modeParam->load());

    // Update target coefficients
    updateSmoothingCoefficients();

    // Smooth coefficient changes over time to prevent clicks
    const float paramSlewCoeff = std::exp(-1.0f / (0.01f * static_cast<float>(sampleRate)));
    currentAttackCoeff = paramSlewCoeff * currentAttackCoeff + (1.0f - paramSlewCoeff) * targetAttackCoeff;
    currentReleaseCoeff = paramSlewCoeff * currentReleaseCoeff + (1.0f - paramSlewCoeff) * targetReleaseCoeff;

    // Ensure channel states vector is properly sized
    if (channelStates.size() != totalNumInputChannels)
    {
        channelStates.resize(totalNumInputChannels);
    }

    // Track overall peak for display
    float blockPeak = 0.0f;
    float overallEnvelopeSum = 0.0f;

    // Process each channel
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        auto& state = channelStates[channel];

        // Initialize peak release coefficient from release parameter
        const float releaseTimeS = releaseParam->load() * 0.001f;
        state.peakReleaseCoeff = std::exp(-1.0f / (releaseTimeS * 10.0f * static_cast<float>(sampleRate)));

        for (int i = 0; i < numSamples; ++i)
        {
            float input = channelData[i];

            // Update delay line
            hilbertBuffer[bufferIndex] = input;

            // Compute Hilbert transform (90° phase shift)
            float hilbert = 0.0f;
            for (int n = 0; n < filterTaps; ++n)
            {
                int idx = (bufferIndex + n) % filterTaps;
                hilbert += hilbertCoeffs[n] * hilbertBuffer[idx];
            }

            // Compute instantaneous envelope
            float instantaneousEnvelope = std::sqrt(input * input + hilbert * hilbert);

            // Apply mode-specific processing
            float envelopeToUse = instantaneousEnvelope;

            if (mode == 1 || mode == 2)  // Smoothed or Sidechain modes
            {
                // Apply attack/release smoothing
                state.smoothedEnvelope = processEnvelopeSmoothing(
                    instantaneousEnvelope,
                    state.smoothedEnvelope,
                    currentAttackCoeff,
                    currentReleaseCoeff
                );
                envelopeToUse = state.smoothedEnvelope;
            }

            // Update peak detector
            if (envelopeToUse > state.peakHold)
            {
                state.peakHold = envelopeToUse;
            }
            else
            {
                state.peakHold *= state.peakReleaseCoeff;
            }

            // Track block peak for display
            if (envelopeToUse > blockPeak)
                blockPeak = envelopeToUse;

            // Sum for overall display (average across channels)
            overallEnvelopeSum += envelopeToUse;

            // Create output based on mode
            float output;
            if (mode == 2)  // Sidechain mode: output ONLY the envelope
            {
                output = envelopeToUse * 0.707f * gain;  // -3dB scaling
            }
            else  // Instant or Smoothed mode: modulate the dry signal
            {
                output = createOutput(input, envelopeToUse, mix, gain);
            }

            // Apply final soft clipping
            output = std::tanh(output);

            channelData[i] = output;

            // Push samples to scope (every 10 samples for CPU)
            if (i % 10 == 0 && channel == 0)  // Only left channel for scope
            {
                pushScopeSample(envelopeToUse, state.peakHold);
            }

            // Advance delay line
            bufferIndex = (bufferIndex + 1) % filterTaps;
        }
    }

    // Update atomic variables for GUI
    if (totalNumInputChannels > 0)
    {
        float averageEnvelope = overallEnvelopeSum / (totalNumInputChannels * numSamples);
        currentEnvelope.store(averageEnvelope);
    }
    else
    {
        currentEnvelope.store(0.0f);
    }

    // Update peak envelope
    peakEnvelope.store(blockPeak);
}

juce::AudioProcessorEditor* HilbertEnvelopeProcessor::createEditor()
{
    return new HilbertEnvelopeEditor(*this);
}

void HilbertEnvelopeProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void HilbertEnvelopeProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// Factory function
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HilbertEnvelopeProcessor();
}