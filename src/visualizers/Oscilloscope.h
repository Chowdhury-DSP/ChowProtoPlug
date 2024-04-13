#pragma once

#include <pch.h>

namespace viz
{
struct ScopeBackgroundTask : chowdsp::TimeSliceAudioUIBackgroundTask
{
    ScopeBackgroundTask() : chowdsp::TimeSliceAudioUIBackgroundTask ("Oscilloscope Background Task") {}

    void prepareTask (double sampleRate, int samplesPerBlock, int& requstedBlockSize, int& waitMs) override;
    void resetTask() override;
    void runTask (const juce::AudioBuffer<float>& data) override;

    juce::Point<float> mapXY (int sampleIndex, float yVal) const;
    void setBounds (juce::Rectangle<int> newBounds);
    juce::Path getScopePath() const noexcept;

private:
    juce::CriticalSection crit;
    juce::Path scopePath;
    juce::Rectangle<float> bounds {};

    int samplesToDisplay = 0;
    int triggerBuffer = 0;
};

struct ScopeComponent : public juce::Component, private juce::Timer
{
    explicit ScopeComponent (ScopeBackgroundTask& sTask);
    ~ScopeComponent() override;

    void enablementChanged() override;
    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    ScopeBackgroundTask& scopeTask;
};
} // namespace viz
