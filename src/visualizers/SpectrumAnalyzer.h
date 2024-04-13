#pragma once

#include <pch.h>

namespace viz
{
struct Spectrum_Analyser : chowdsp::TimeSliceAudioUIBackgroundTask
{
    Spectrum_Analyser() : chowdsp::TimeSliceAudioUIBackgroundTask ("Spectrum Analyser Background Task") {}

    void prepareTask (double sampleRate, [[maybe_unused]] int samplesPerBlock, int& requestedBlockSize, int& waitMs) override;
    void resetTask() override;
    void runTask (const juce::AudioBuffer<float>& data) override;

    juce::CriticalSection mutex {};
    std::vector<float> fft_freqs {};
    std::vector<float> fft_mags_smoothed_db {};

    static constexpr float min_db = -30.0f;
    static constexpr float max_db = 30.0f;

private:
    std::optional<juce::dsp::FFT> fft {};
    std::optional<juce::dsp::WindowingFunction<float>> window {};

    int fft_size = 0;
    int fft_data_size = 0;
    int fft_out_size = 0;

    chowdsp::Buffer<float> scratch_mono_buffer {};
    std::vector<float> fft_mags_unsmoothed_db {};
    std::vector<float> mags_previous {};
};

class Spectrum_Display : public chowdsp::SpectrumPlotBase,
                         public juce::Timer
{
public:
    Spectrum_Display (Spectrum_Analyser& input_spectrum, Spectrum_Analyser& output_spectrum);
    ~Spectrum_Display() override;

    void paint (juce::Graphics& g) override;
    void visibilityChanged() override;
    void timerCallback() override;

    void update_plot_path (juce::Path& path_to_update, Spectrum_Analyser& analyzer);

    struct Draw_Options
    {
        bool draw_fill = false;
        bool draw_line = false;
        juce::Colour gradient_start_colour = juce::Colours::red;
        juce::Colour gradient_end_colour = juce::Colours::red;
        juce::Colour line_colour = juce::Colours::yellow;
    };

    Draw_Options input_spectrum_draw_options { .draw_line = true };
    Draw_Options output_spectrum_draw_options { .draw_fill = true };

private:
    juce::Path input_spectrum_path;
    juce::Path output_spectrum_path;

    Spectrum_Analyser& input_spectrum;
    Spectrum_Analyser& output_spectrum;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Spectrum_Display)
};
} // namespace viz
