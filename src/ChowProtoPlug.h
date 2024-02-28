#pragma once

#include <pch.h>

#include "HotReloadedModule.h"
#include "Logger.h"

class ChowProtoPlug : public chowdsp::PluginBase<chowdsp::PluginStateImpl<chowdsp::ParamHolder>>
{
public:
    ChowProtoPlug();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processAudioBlock (juce::AudioBuffer<float>& buffer) override;

    juce::AudioProcessorEditor* createEditor() override;

    void update_config();

    Logger logger;

private:
    ModuleConfig config {};
    HotReloadedModule module;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChowProtoPlug)
};
