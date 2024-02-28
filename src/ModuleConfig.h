#pragma once

#include <pch.h>

struct ModuleConfig
{
    inline const static auto config_file = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("ChowdhuryDSP/ChowProtoPlug/config.json");
    std::string cmake_path {};
    std::string module_directory {};
    std::string module_name {};
};
