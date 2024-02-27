#include "ChowProtoPlug.h"

ChowProtoPlug::ChowProtoPlug()
{
    const auto fallback_config = [this]
    {
#if JUCE_WINDOWS
        config.cmake_path ="C:/Program Files/JetBrains/CLion 2023.2/bin/cmake/win/x64/bin/cmake.exe";
#elif JUCE__MAC
        config.cmake_path ="/opt/homebrew/bin/cmake";
#endif
        config.module_directory = DEFAULT_MODULE_PATH;
        ModuleConfig::config_file.create();
        auto ec = glz::write_file_json (config, ModuleConfig::config_file.getFullPathName().toStdString(), std::string{});
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

    module.emplace (config);
}

void ChowProtoPlug::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    setRateAndBufferSizeDetails (sampleRate, samplesPerBlock);

    module->prepare ({
        sampleRate,
        static_cast<uint32_t> (samplesPerBlock),
        static_cast<uint32_t> (getMainBusNumInputChannels()),
    });
}

void ChowProtoPlug::processAudioBlock (juce::AudioBuffer<float>& buffer)
{
    module->process (buffer);
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
