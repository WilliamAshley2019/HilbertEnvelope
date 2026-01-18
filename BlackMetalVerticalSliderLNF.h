// BlackMetalVerticalSliderLNF.h
#pragma once
#include <JuceHeader.h>

// =======================================
// Black Metal Vertical Slider LookAndFeel
// =======================================
class BlackMetalVerticalSliderLNF : public juce::LookAndFeel_V4
{
public:
    void drawLinearSlider(juce::Graphics& g,
        int x, int y, int width, int height,
        float sliderPos, [[maybe_unused]] float minSliderPos, [[maybe_unused]] float maxSliderPos,
        const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float>(
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(width),
            static_cast<float>(height)).reduced(2.0f);

        const float sliderWidth = bounds.getWidth() * 0.6f;
        const float trackWidth = sliderWidth * 0.3f;
        const float centerX = bounds.getCentreX();

        // Draw track background (vertical)
        g.setColour(juce::Colour(15, 15, 15));
        g.fillRoundedRectangle(centerX - trackWidth * 0.5f, bounds.getY(),
            trackWidth, bounds.getHeight(), trackWidth * 0.5f);

        // Draw filled portion (bottom to top)
        const float fillHeight = (1.0f - sliderPos) * bounds.getHeight();
        if (fillHeight > 0)
        {
            juce::ColourGradient grad(
                juce::Colour(60, 60, 60),
                centerX, bounds.getBottom(),
                juce::Colour(25, 25, 25),
                centerX, bounds.getBottom() - fillHeight,
                false);

            g.setGradientFill(grad);
            g.fillRoundedRectangle(centerX - trackWidth * 0.5f,
                bounds.getBottom() - fillHeight,
                trackWidth, fillHeight,
                trackWidth * 0.5f);
        }

        // Draw slider thumb
        const float thumbWidth = sliderWidth * 1.2f;
        const float thumbHeight = sliderWidth * 0.8f;
        const float thumbX = centerX - thumbWidth * 0.5f;
        const float thumbY = bounds.getY() + sliderPos * bounds.getHeight() - thumbHeight * 0.5f;

        juce::ColourGradient thumbGrad(
            juce::Colour(45, 45, 45),
            thumbX + thumbWidth * 0.5f, thumbY,
            juce::Colour(10, 10, 10),
            thumbX + thumbWidth * 0.5f, thumbY + thumbHeight,
            false);

        g.setGradientFill(thumbGrad);
        g.fillRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, 2.0f);

        // Draw thumb outline
        g.setColour(juce::Colour(80, 80, 80));
        g.drawRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, 2.0f, 1.0f);

        // Draw center indicator line
        g.setColour(juce::Colour(100, 100, 100));
        g.drawLine(thumbX, bounds.getCentreY(),
            thumbX + thumbWidth, bounds.getCentreY(), 0.5f);
    }

    // Optional: Customize thumb size
    int getSliderThumbRadius(juce::Slider& slider) override
    {
        return 8;
    }
};