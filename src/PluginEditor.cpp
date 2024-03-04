#include "PluginEditor.h"
#include "ChowProtoPlug.h"

PluginEditor::PluginEditor (ChowProtoPlug& plug)
    : AudioProcessorEditor { plug },
      plugin { plug },
      tabs { juce::TabbedButtonBar::Orientation::TabsAtTop }
{
    console_tab.console_log.setMultiLine (true);
    console_tab.console_log.setFont (juce::Font { "JetBrains Mono", 16.0f, juce::Font::plain });
    console_tab.console_log.setReadOnly (true);
    plugin.logger.set_console (&console_tab.console_log);
    console_tab.addAndMakeVisible (console_tab.console_log);

    console_tab.clear_logs_button.onClick = [this]
    {
        plugin.logger.log_text.clear();
        plugin.logger.update_console();
    };
    console_tab.addAndMakeVisible (console_tab.clear_logs_button);

    tabs.addTab ("Console", juce::Colours::black, &console_tab, false);

    addAndMakeVisible (tabs);

    settings_button.onClick = [this]
    {
        juce::PopupMenu menu;

        menu.addItem ("Open Settings", [] { ModuleConfig::config_file.getParentDirectory().startAsProcess(); });
        menu.addItem ("Reload Settings", [this] { plugin.update_config(); });

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&settings_button));
    };
    addAndMakeVisible (settings_button);

    recompile_button.onClick = [this]
    {
        plugin.module.dll_source_file_changed();
    };
    addAndMakeVisible (recompile_button);

    setSize (800, 600);
    setResizable (true, true);
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

    tabs.setBounds (b.removeFromTop (proportionOfHeight (0.95f)).reduced (5));
    settings_button.setBounds (b.removeFromLeft (proportionOfWidth (0.15f)).reduced (2));
    recompile_button.setBounds (b.removeFromLeft (proportionOfWidth (0.15f)).reduced (2));
}
