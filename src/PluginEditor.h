#pragma once

#include <pch.h>

class ChowProtoPlug;
struct PluginEditor : juce::AudioProcessorEditor
{
    explicit PluginEditor (ChowProtoPlug& plugin);
    ~PluginEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    ChowProtoPlug& plugin;

    juce::TextEditor console_log {};
    juce::TextButton settings_button { "SETTINGS" };
    juce::TextButton clear_logs_button { "CLEAR LOGS" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
