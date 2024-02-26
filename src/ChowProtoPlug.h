#pragma once

#include <pch.h>

struct Params : chowdsp::ParamHolder
{
};

using State = chowdsp::PluginStateImpl<Params>;

class ChowProtoPlug : public chowdsp::PluginBase<State>
{
public:
    ChowProtoPlug();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processAudioBlock (juce::AudioBuffer<float>& buffer) override;

    juce::AudioProcessorEditor* createEditor() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChowProtoPlug)
};
