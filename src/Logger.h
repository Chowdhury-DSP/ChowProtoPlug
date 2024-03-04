#pragma once

#include <pch.h>

struct Logger : juce::Logger
{
    Logger()
    {
        setCurrentLogger (this);
    }

    ~Logger() override
    {
        setCurrentLogger (nullptr);
    }

    void logMessage (const juce::String& message) override
    {
        base_logger.logMessage (message);
        log_text += message.toStdString() + "\n";
        update_console();
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
    chowdsp::BaseLogger base_logger;
};
