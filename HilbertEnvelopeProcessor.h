// HilbertEnvelopeProcessor.h - UPDATED VERSION WITH FIXED MATH
#pragma once

#include <JuceHeader.h>

class HilbertEnvelopeProcessor : public juce::AudioProcessor
{
public:
    HilbertEnvelopeProcessor();
    ~HilbertEnvelopeProcessor() override;

    //==============================================================================
    // Required JUCE AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Hilbert Envelope"; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        if (layouts.getMainInputChannelSet() == juce::AudioChannelSet::disabled()
            || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::disabled())
            return false;

        return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
    }

    //==============================================================================
    // Public interface
    float getCurrentEnvelope() const { return currentEnvelope.load(); }
    float getPeakEnvelope() const { return peakEnvelope.load(); }
    void resetPeak() { peakEnvelope = 0.0f; }

    // For scope visualization
    void pushScopeSample(float env, float peak);

    juce::AudioProcessorValueTreeState& getValueTreeState() { return parameters; }

private:
    // Audio processing
    void initializeHilbertFilter();
    void updateSmoothingCoefficients();

    // Smoothing filter for envelope
    float processEnvelopeSmoothing(float input, float currentState,
        float attackCoeff, float releaseCoeff);

    // Output creation with proper mixing
    float createOutput(float input, float envelope, float mix, float gain);

    // Hilbert transform
    std::vector<float> hilbertBuffer;
    int bufferIndex = 0;
    std::vector<float> hilbertCoeffs;
    int filterTaps = 31;  // FIXED: Should be 31, not 32

    // Envelope tracking
    std::atomic<float> currentEnvelope{ 0.0f };
    std::atomic<float> peakEnvelope{ 0.0f };

    // For scope visualization
    std::atomic<float> scopeCurrentEnvelope{ 0.0f };
    std::atomic<float> scopePeakEnvelope{ 0.0f };

    // Per-channel smoothing states
    struct ChannelState
    {
        float smoothedEnvelope = 0.0f;
        float peakHold = 0.0f;
        float peakReleaseCoeff = 0.999f;
    };
    std::vector<ChannelState> channelStates;

    // Smoothing coefficients
    float currentAttackCoeff = 0.0f;
    float currentReleaseCoeff = 0.0f;
    float targetAttackCoeff = 0.0f;
    float targetReleaseCoeff = 0.0f;

    // Parameters
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* gainParam = nullptr;
    std::atomic<float>* attackParam = nullptr;
    std::atomic<float>* releaseParam = nullptr;
    std::atomic<float>* modeParam = nullptr;

    double sampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HilbertEnvelopeProcessor)
};