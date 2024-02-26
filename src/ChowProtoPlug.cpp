#include "ChowProtoPlug.h"

ChowProtoPlug::ChowProtoPlug() = default;

void ChowProtoPlug::prepareToPlay ([[maybe_unused]] double sampleRate, [[maybe_unused]] int samplesPerBlock)
{
}

void ChowProtoPlug::processAudioBlock ([[maybe_unused]] juce::AudioBuffer<float>& buffer)
{
}

juce::AudioProcessorEditor* ChowProtoPlug::createEditor()
{
    return new chowdsp::ParametersViewEditor { *this, state, state.params };
}

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChowProtoPlug();
}
