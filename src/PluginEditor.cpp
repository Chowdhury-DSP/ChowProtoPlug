#include "PluginEditor.h"
#include "ChowProtoPlug.h"

PluginEditor::PluginEditor (ChowProtoPlug& plug)
    : AudioProcessorEditor { plug },
      plugin { plug }
{
    console_log.setMultiLine (true);
    console_log.setFont (juce::Font { "JetBrains Mono", 16.0f, juce::Font::plain });
    console_log.setReadOnly (true);
    plugin.logger.set_console (&console_log);
    addAndMakeVisible (console_log);

    settings_button.onClick = [this]
    {
        juce::PopupMenu menu;

        menu.addItem ("Open Settings", [] { ModuleConfig::config_file.getParentDirectory().startAsProcess(); });
        menu.addItem ("Reload Settings", [this] { plugin.update_config(); });

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&settings_button));
    };
    addAndMakeVisible (settings_button);

    clear_logs_button.onClick = [this]
    {
        plugin.logger.log_text.clear();
        plugin.logger.update_console();
    };
    addAndMakeVisible (clear_logs_button);

    setSize (800, 600);
}

PluginEditor::~PluginEditor()
{
    plugin.logger.set_console (nullptr);
}

void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void PluginEditor::resized()
{
    auto b = getLocalBounds();
    console_log.setBounds (b.removeFromTop (proportionOfHeight (0.95f)).reduced (5));
    settings_button.setBounds (b.removeFromLeft (proportionOfWidth (0.15f)).reduced (2));
    clear_logs_button.setBounds (b.removeFromLeft (proportionOfWidth (0.15f)).reduced (2));
}
