#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout HeatProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "heat", 1 }, "Heat",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));
    return layout;
}

HeatProcessor::HeatProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    heatParam = apvts.getRawParameterValue ("heat");
}

bool HeatProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    auto out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && out == l.getMainInputChannelSet();
}

void HeatProcessor::prepareToPlay (double sampleRate, int)
{
    smoothedHeat.reset (sampleRate, 0.03);
    smoothedHeat.setCurrentAndTargetValue (heatParam->load());
}

void HeatProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh = juce::jmin (getTotalNumInputChannels(), buffer.getNumChannels());
    const int n = buffer.getNumSamples();

    for (int c = numCh; c < buffer.getNumChannels(); ++c)
        buffer.clear (c, 0, n);

    smoothedHeat.setTargetValue (heatParam->load());

    for (int i = 0; i < n; ++i)
    {
        const float h = smoothedHeat.getNextValue() * 0.01f;      // 0..1
        const float gain = juce::Decibels::decibelsToGain (h * 45.0f);   // 0..+45 dB drive
        const float k = h * h;                                    // soft -> hard clip blend
        const float outGain = juce::jmap (h, 1.0f, 0.6f);
        const float mix = juce::jmin (1.0f, h * 10.0f);           // %0 = tamamen temiz

        for (int c = 0; c < numCh; ++c)
        {
            float* d = buffer.getWritePointer (c);
            const float dry = d[i];
            const float y = dry * gain;
            const float shaped = (1.0f - k) * std::tanh (y) + k * juce::jlimit (-1.0f, 1.0f, y);
            d[i] = dry * (1.0f - mix) + shaped * outGain * mix;
        }
    }
}

juce::AudioProcessorEditor* HeatProcessor::createEditor() { return new HeatEditor (*this); }

void HeatProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void HeatProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new HeatProcessor(); }
