#pragma once

#include <pch.h>

#include "HotReloadedModule.h"

class ChowProtoPlug : public chowdsp::PluginBase<chowdsp::PluginStateImpl<chowdsp::ParamHolder>>
{
public:
    ChowProtoPlug();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processAudioBlock (juce::AudioBuffer<float>& buffer) override;

    juce::AudioProcessorEditor* createEditor() override;

private:
    ModuleConfig config {};
    std::optional<HotReloadedModule> module;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChowProtoPlug)
};
