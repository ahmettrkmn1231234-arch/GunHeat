#pragma once
#include "PluginProcessor.h"

class HeatEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit HeatEditor (HeatProcessor&);
    ~HeatEditor() override { stopTimer(); }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Particle { float x, y, vx, vy, life, size; bool spark; };

    void timerCallback() override;
    static juce::Colour heatColour (float h);

    HeatProcessor& proc;
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;

    std::vector<Particle> particles;
    juce::Random rnd;
    float heat = 0.0f, time = 0.0f;

    static constexpr float gunScale = 1.3333f, gunX = 50.0f, gunY = 50.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeatEditor)
};
