#pragma once

#include <pch.h>

struct Logger : chowdsp::BaseLogger
{
    Logger()
    {
        onLogMessage.connect ([this] (const juce::String& message)
        {
            log_text += message.toStdString() + "\n";
            update_console();
        });
        chowdsp::set_global_logger (this);
    }

    ~Logger() override
    {
        chowdsp::set_global_logger (nullptr);
    }

    void set_console (juce::TextEditor* new_console)
    {
        console = new_console;
        update_console();
    }

    void update_console() const
    {
        if (console != nullptr)
        {
            console->setText (log_text, juce::sendNotification);
            console->moveCaretToEnd();
        }
    }

    std::string log_text {};
    juce::TextEditor* console { nullptr };
};
