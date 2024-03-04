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

    juce::TextButton recompile_button { "RECOMPILE" };
    juce::TextButton settings_button { "SETTINGS" };

    struct ConsoleTab : juce::Component
    {
        juce::TextEditor console_log {};
        juce::TextButton clear_logs_button { "CLEAR LOGS" };

        void resized() override
        {
            auto b = getLocalBounds();
            console_log.setBounds (b);
            clear_logs_button.setBounds (b.removeFromBottom (30).removeFromRight (50));
        }
    } console_tab;

    juce::TabbedComponent tabs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
