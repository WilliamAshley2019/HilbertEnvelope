// EnvelopeScope.h
#pragma once
#include <JuceHeader.h>

class EnvelopeScope : public juce::Component, private juce::Timer
{
public:
    EnvelopeScope()
    {
        startTimerHz(30);  // Lower refresh rate for CPU
        history.resize(300, 0.0f);
        peakHistory.resize(300, 0.0f);

        // Initialize with zeros
        std::fill(history.begin(), history.end(), 0.0f);
        std::fill(peakHistory.begin(), peakHistory.end(), 0.0f);
    }

    ~EnvelopeScope()
    {
        stopTimer();
    }

    void pushSample(float envelopeValue, float peakHoldValue)
    {
        // Add some hysteresis to prevent flickering
        envelopeValue = juce::jlimit(0.0f, 1.0f, envelopeValue);
        peakHoldValue = juce::jlimit(0.0f, 1.0f, peakHoldValue);

        history[index] = envelopeValue;
        peakHistory[index] = peakHoldValue;
        index = (index + 1) % history.size();

        // Trigger repaint
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        // Dark background
        g.fillAll(juce::Colour(20, 22, 25));

        // Draw border
        g.setColour(juce::Colour(50, 52, 58));
        g.drawRect(getLocalBounds(), 2);

        // Draw grid lines
        g.setColour(juce::Colour(35, 37, 42));

        // Horizontal grid (amplitude)
        for (int i = 1; i < 10; ++i)
        {
            float y = getHeight() * i / 10.0f;
            g.drawHorizontalLine(static_cast<int>(y), 0, getWidth());
        }

        // Vertical grid (time)
        for (int i = 1; i < 10; ++i)
        {
            float x = getWidth() * i / 10.0f;
            g.drawVerticalLine(static_cast<int>(x), 0, getHeight());
        }

        // Draw 0dB line at 0.0 (top)
        g.setColour(juce::Colour(80, 80, 80).withAlpha(0.3f));
        g.drawHorizontalLine(0, 0, getWidth());

        // Draw -6dB line
        float db6Line = getHeight() * 0.5f;  // 0.5 amplitude = -6dB
        g.setColour(juce::Colour(60, 60, 60).withAlpha(0.2f));
        g.drawHorizontalLine(static_cast<int>(db6Line), 0, getWidth());

        // Draw envelope waveform
        juce::Path envelopePath;
        juce::Path peakPath;
        bool envelopeStarted = false;
        bool peakStarted = false;

        for (size_t i = 0; i < history.size(); ++i)
        {
            size_t idx = (index + i) % history.size();
            float x = static_cast<float>(i) / history.size() * getWidth();

            // Convert amplitude to Y coordinate (0 at top, 1 at bottom)
            float envY = getHeight() * (1.0f - history[idx]);
            float peakY = getHeight() * (1.0f - peakHistory[idx]);

            // Clamp to bounds
            envY = juce::jlimit(0.0f, static_cast<float>(getHeight()), envY);
            peakY = juce::jlimit(0.0f, static_cast<float>(getHeight()), peakY);

            if (i == 0)
            {
                envelopePath.startNewSubPath(x, envY);
                peakPath.startNewSubPath(x, peakY);
            }
            else
            {
                envelopePath.lineTo(x, envY);
                peakPath.lineTo(x, peakY);
            }
        }

        // Draw envelope path (cyan)
        g.setColour(juce::Colour(100, 200, 255));
        g.strokePath(envelopePath, juce::PathStrokeType(2.0f));

        // Draw peak path (red)
        g.setColour(juce::Colour(255, 100, 100).withAlpha(0.7f));
        g.strokePath(peakPath, juce::PathStrokeType(1.0f));

        // Draw labels
        g.setColour(juce::Colour(180, 180, 180));
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));

        // Amplitude labels on left
        g.drawText("0 dB", 5, 0, 40, 20, juce::Justification::left);
        g.drawText("-6 dB", 5, static_cast<int>(db6Line) - 10, 40, 20, juce::Justification::left);
        g.drawText("-∞", 5, getHeight() - 20, 40, 20, juce::Justification::left);

        // Legend
        g.setFont(juce::FontOptions(11.0f, juce::Font::plain));
        g.setColour(juce::Colour(100, 200, 255));
        g.drawText("Envelope", getWidth() - 80, 5, 75, 15, juce::Justification::right);
        g.setColour(juce::Colour(255, 100, 100));
        g.drawText("Peak Hold", getWidth() - 80, 25, 75, 15, juce::Justification::right);
    }

private:
    void timerCallback() override
    {
        repaint();
    }

    std::vector<float> history;
    std::vector<float> peakHistory;
    size_t index = 0;
};