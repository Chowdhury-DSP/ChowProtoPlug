#include "Oscilloscope.h"

namespace viz
{
constexpr int scope_fps = 30;

void ScopeBackgroundTask::prepareTask (double sampleRate, int /*samplesPerBlock*/, int& requstedBlockSize, int& waitMs)
{
    constexpr auto millisecondsToDisplay = 20.0;
    samplesToDisplay = int (millisecondsToDisplay * 0.001 * sampleRate) - 1;
    triggerBuffer = int (sampleRate / 50.0);

    requstedBlockSize = samplesToDisplay + triggerBuffer;

    waitMs = int (1000.0 / (double) scope_fps);
}

void ScopeBackgroundTask::resetTask()
{
    juce::ScopedLock sl (crit);
    scopePath.clear();
    scopePath.startNewSubPath (mapXY (0, 0.0f));
    scopePath.lineTo (mapXY (samplesToDisplay, 0.0f));
}

juce::Point<float> ScopeBackgroundTask::mapXY (int sampleIndex, float yVal) const
{
    return juce::Point { juce::jmap (float (sampleIndex), 0.0f, float (samplesToDisplay), bounds.getX(), bounds.getRight()),
                         juce::jmap (yVal, -1.0f, 1.0f, bounds.getBottom(), bounds.getY()) };
}

void ScopeBackgroundTask::runTask (const juce::AudioBuffer<float>& buffer)
{
    const auto* data = buffer.getReadPointer (0);

    // trigger from last zero-crossing
    int triggerOffset = triggerBuffer - 1;
    auto sign = data[triggerOffset] > 0.0f;

    while (! sign && triggerOffset > 0)
        sign = data[triggerOffset--] > 0.0f;

    while (sign && triggerOffset > 0)
        sign = data[triggerOffset--] > 0.0f;

    // update path
    juce::ScopedLock sl (crit);
    if (bounds == juce::Rectangle<float> {})
        return;

    scopePath.clear();
    scopePath.startNewSubPath (mapXY (0, data[triggerOffset]));
    for (int i = 1; i < samplesToDisplay; ++i)
        scopePath.lineTo (mapXY (i, data[triggerOffset + i]));
}

void ScopeBackgroundTask::setBounds (juce::Rectangle<int> newBounds)
{
    juce::ScopedLock sl (crit);
    bounds = newBounds.toFloat();
}

juce::Path ScopeBackgroundTask::getScopePath() const noexcept
{
    juce::ScopedLock sl (crit);
    return scopePath;
}

//=================================================================================================
ScopeComponent::ScopeComponent (ScopeBackgroundTask& sTask) : scopeTask (sTask)
{
    scopeTask.setShouldBeRunning (true);
    startTimerHz (scope_fps);
}

ScopeComponent::~ScopeComponent()
{
    scopeTask.setShouldBeRunning (false);
}

void ScopeComponent::enablementChanged()
{
    scopeTask.setShouldBeRunning (isEnabled());
}

void ScopeComponent::paint (juce::Graphics& g)
{
    auto b = getLocalBounds();

    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (b.toFloat(), 15.0f);

    constexpr float lineThickness = 2.0f;
    g.setColour (juce::Colours::red.withAlpha (isEnabled() ? 1.0f : 0.6f));
    auto scopePath = scopeTask.getScopePath();
    g.strokePath (scopePath, juce::PathStrokeType (lineThickness));
}

void ScopeComponent::resized()
{
    scopeTask.setBounds (getLocalBounds());
}

void ScopeComponent::timerCallback()
{
    repaint();
}
}
