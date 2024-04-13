#pragma once

#include <pch.h>

#include "visualizers/Oscilloscope.h"

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

    struct ParamsTab : juce::Component
    {
        chowdsp::ScopedCallback params_changed_callback;
        std::optional<chowdsp::ParametersView> params_view;

        void params_cleared()
        {
            params_view.reset();
        }

        void params_added (ModuleParams& params)
        {
            params_view.emplace (*params.param_listeners, params);
            addAndMakeVisible (*params_view);
            resized();
        }

        void resized() override
        {
            if (params_view.has_value())
                params_view->setBounds(getLocalBounds());
        }
    } params_tab;

    struct VizTab : juce::Component
    {
        std::optional<viz::ScopeComponent> scope;
        std::optional<viz::Spectrum_Display> spectrum;

        void paint (juce::Graphics& g) override
        {
            g.fillAll (juce::Colours::darkgrey);
        }

        void resized() override
        {
            auto b = getLocalBounds();
            const auto viz_row_height = proportionOfHeight (0.333f);
            const auto pad = proportionOfWidth (0.005f);

            if (scope.has_value())
                scope->setBounds (b.removeFromTop (viz_row_height).reduced (pad));
            if (spectrum.has_value())
                spectrum->setBounds (b.removeFromTop (viz_row_height).reduced (pad));
        }
    } viz_tab;

    juce::TabbedComponent tabs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
