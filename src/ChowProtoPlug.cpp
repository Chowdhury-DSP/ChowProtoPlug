#include "ChowProtoPlug.h"

ChowProtoPlug::ChowProtoPlug() = default;

void ChowProtoPlug::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    module.prepare ({
        sampleRate,
        static_cast<uint32_t> (samplesPerBlock),
        static_cast<uint32_t> (getMainBusNumInputChannels()),
    });
}

void ChowProtoPlug::processAudioBlock (juce::AudioBuffer<float>& buffer)
{
    module.process (buffer);
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
