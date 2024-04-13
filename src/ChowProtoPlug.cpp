#include "ChowProtoPlug.h"
#include "PluginEditor.h"

ChowProtoPlug::ChowProtoPlug()
{
    params.forwarding_params.emplace (*this, state);
    module.params = &params;

    update_config();
}

void ChowProtoPlug::update_config()
{
    const auto fallback_config = [this]
    {
        config.module_directory = DEFAULT_MODULE_PATH;
        config.module_name = "ProtoPlugTestModule";
        ModuleConfig::config_file.create();
        [[maybe_unused]] const auto ec = glz::write_file_json (config, ModuleConfig::config_file.getFullPathName().toStdString(), std::string{});
        jassert (! ec);
    };

    if (ModuleConfig::config_file.existsAsFile())
    {
        auto ec = glz::read_file_json (config, ModuleConfig::config_file.getFullPathName().toStdString(), std::string{});
        if (ec)
            fallback_config();
    }
    else
    {
        fallback_config();
    }

    module.update_config (config);
    module.dll_source_file_changed();
}

void ChowProtoPlug::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    setRateAndBufferSizeDetails (sampleRate, samplesPerBlock);

    module.prepare ({
        sampleRate,
        static_cast<uint32_t> (samplesPerBlock),
        static_cast<uint32_t> (getMainBusNumInputChannels()),
    });

    scope_task.prepare (sampleRate, samplesPerBlock, getMainBusNumInputChannels());
    input_spectrum.prepare (sampleRate, samplesPerBlock, getMainBusNumInputChannels());
    output_spectrum.prepare (sampleRate, samplesPerBlock, getMainBusNumInputChannels());
}

void ChowProtoPlug::processAudioBlock (juce::AudioBuffer<float>& buffer)
{
    input_spectrum.pushSamples (buffer);

    module.process (buffer);

    scope_task.pushSamples (buffer);
    output_spectrum.pushSamples (buffer);
}

juce::AudioProcessorEditor* ChowProtoPlug::createEditor()
{
    return new PluginEditor { *this };
}

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChowProtoPlug();
}
