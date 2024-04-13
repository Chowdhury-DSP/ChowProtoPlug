#include "SpectrumAnalyzer.h"

namespace viz
{
static std::vector<float> get_fft_freqs (int N, float T)
{
    auto val = 0.5f / ((float) N * T);

    std::vector<float> results ((size_t) N, 0.0f);
    std::iota (results.begin(), results.end(), 0.0f);
    std::transform (results.begin(), results.end(), results.begin(), [val] (auto x)
                    { return x * val; });

    return results;
}

[[maybe_unused]] static void freq_smooth (const float* inData, float* outData, int numSamples, float smFactor = 1.0f / 24.0f)
{
    const auto s = smFactor > 1.0f ? smFactor : std::sqrt (std::exp2 (smFactor));
    for (int i = 0; i < numSamples; ++i)
    {
        auto i1 = std::max (int ((float) i / s), 0);
        auto i2 = std::min (int ((float) i * s) + 1, numSamples - 1);

        //        outData[i] = i2 > i1 ? std::accumulate (inData + i1, inData + i2, 0.0f) / std::pow (float (i2 - i1), 1.0f) : 0.0f;
        outData[i] = *std::max_element (inData + i1, inData + i2);
    }
}

//alternative to moving average
[[maybe_unused]] static void exp_smooth (float* zData, float* outData, int numSamples, float alpha)
{
    for (int i = 0; i < numSamples; ++i)
    {
        outData[i] = alpha * outData[i] + (1 - alpha) * zData[i];
        zData[i] = outData[i];
    }
}

void Spectrum_Analyser::prepareTask (double sampleRate, [[maybe_unused]] int samplesPerBlock, int& requestedBlockSize, int& waitMs)
{
    static constexpr auto maxBinWidth = 6.0;
    fft_size = juce::nextPowerOfTwo (int (sampleRate / maxBinWidth));

    fft.emplace (chowdsp::Math::log2 (fft_size));
    window.emplace ((size_t) fft_size, juce::dsp::WindowingFunction<float>::WindowingMethod::triangular);

    fft_data_size = fft_size * 2;
    fft_out_size = fft_size / 2 + 1;

    requestedBlockSize = fft_size;
    waitMs = 10;

    scratch_mono_buffer.setMaxSize (1, fft_data_size);
    fft_freqs = get_fft_freqs (fft_out_size, 1.0f / (float) sampleRate);
    fft_mags_unsmoothed_db = std::vector<float> ((size_t) fft_out_size, 0.0f);
    fft_mags_smoothed_db = std::vector<float> ((size_t) fft_out_size, 0.0f);
    mags_previous = std::vector<float> ((size_t) fft_out_size, 0.0f);
}

static constexpr auto spectrum_minus_inf_db = -100.0f;
void Spectrum_Analyser::resetTask()
{
    const juce::CriticalSection::ScopedLockType lock { mutex };
    std::fill (fft_mags_smoothed_db.begin(), fft_mags_smoothed_db.end(), spectrum_minus_inf_db);
    std::fill (mags_previous.begin(), mags_previous.end(), spectrum_minus_inf_db);
}

void Spectrum_Analyser::runTask (const juce::AudioBuffer<float>& data)
{
    jassert (data.getNumSamples() == fft_size);

    scratch_mono_buffer.setCurrentSize (1, data.getNumSamples());
    chowdsp::BufferMath::sumToMono (data, scratch_mono_buffer);

    auto* scratchData = scratch_mono_buffer.getWritePointer (0);
    window->multiplyWithWindowingTable (scratchData, (size_t) fft_size);
    fft->performFrequencyOnlyForwardTransform (scratchData, true);

    juce::FloatVectorOperations::multiply (scratchData, 2.0f / (float) fft_out_size, fft_out_size);
    for (size_t i = 0; i < (size_t) fft_out_size; ++i)
        fft_mags_unsmoothed_db[i] = juce::Decibels::gainToDecibels (scratchData[i], spectrum_minus_inf_db);

    auto maxElement = std::max_element (fft_mags_unsmoothed_db.begin(), fft_mags_unsmoothed_db.end());
    if (*maxElement == spectrum_minus_inf_db)
    {
        std::fill (fft_mags_unsmoothed_db.begin(), fft_mags_unsmoothed_db.end(), min_db);
    }
    else
    {
        for (auto& dB : fft_mags_unsmoothed_db)
        {
            dB = juce::jmap (dB,
                             spectrum_minus_inf_db,
                             std::max (*maxElement, max_db - 6.0f),
                             min_db,
                             max_db);
        }
    }

    const juce::CriticalSection::ScopedLockType lock { mutex };
    freq_smooth (fft_mags_unsmoothed_db.data(), fft_mags_smoothed_db.data(), fft_out_size, 1.0f / 128.0f);
    exp_smooth (mags_previous.data(), fft_mags_smoothed_db.data(), fft_out_size, 0.15f);
}

//=================================================================================================
Spectrum_Display::Spectrum_Display (Spectrum_Analyser& input_spec, Spectrum_Analyser& output_spec)
    : SpectrumPlotBase { { .minFrequencyHz = 15.0f, .maxFrequencyHz = 24'000.0f, .minMagnitudeDB = -27.0f, .maxMagnitudeDB = 27.0f } },
      input_spectrum { input_spec },
      output_spectrum { output_spec }
{
    setInterceptsMouseClicks (false, false);
}

Spectrum_Display::~Spectrum_Display()
{
    if (input_spectrum.isTaskRunning())
        input_spectrum.setShouldBeRunning (false);
    if (output_spectrum.isTaskRunning())
        output_spectrum.setShouldBeRunning (false);
}

static constexpr auto minor_frequency_lines()
{
    constexpr size_t start = 2;
    constexpr size_t end = 9;
    constexpr size_t count = end - start;
    std::array<float, count * 3 + 1> freq_lines {};
    for (size_t i = start; i <= end; ++i)
    {
        freq_lines[i - start] = static_cast<float> (10 * i);
        freq_lines[i - start + count] = static_cast<float> (100 * i);
        freq_lines[i - start + 2 * count] = static_cast<float> (1000 * i);
    }
    freq_lines.back() = 20'000.0f;
    return freq_lines;
}

void Spectrum_Display::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);

    // major lines
    g.setColour (juce::Colours::grey);
    drawFrequencyLines (g, { 100.0f, 1000.0f, 10'000.0f }, 2.0f);
    drawMagnitudeLines (g, { -24.0f, -12.0f, 0.0f, 12.0f, 24.0f }, 2.0f);

    // minor lines
    g.setColour (juce::Colours::grey.withAlpha (0.75f));
    drawFrequencyLines (g, minor_frequency_lines(), 1.0f);
    drawMagnitudeLines (g, { -18.0f, -6.0f, 6.0f, 18.0f }, 1.0f);

    // paint spectra
    const auto paint_spectrum = [this, &g] (const Draw_Options& drawOptions, const juce::Path& path)
    {
        if (drawOptions.draw_fill)
        {
            float gradient_start = getYCoordinateForDecibels (-30.0f);
            auto gradient_end = (float) getHeight();

            juce::ColourGradient low_freq_gradient = juce::ColourGradient::vertical (
                drawOptions.gradient_start_colour,
                gradient_start,
                drawOptions.gradient_end_colour,
                gradient_end);
            g.setGradientFill (low_freq_gradient);
            g.fillPath (path);
        }

        if (drawOptions.draw_line)
        {
            g.setColour (drawOptions.line_colour);
            g.strokePath (path, juce::PathStrokeType { 1.1f });
        }
    };

    paint_spectrum (input_spectrum_draw_options, input_spectrum_path);
    paint_spectrum (output_spectrum_draw_options, output_spectrum_path);
}

void Spectrum_Display::visibilityChanged()
{
    if (isVisible())
    {
        input_spectrum.reset();
        input_spectrum.setShouldBeRunning (true);
        output_spectrum.reset();
        output_spectrum.setShouldBeRunning (true);
        startTimerHz (32);
    }
    else
    {
        input_spectrum.setShouldBeRunning (false);
        output_spectrum.setShouldBeRunning (false);
        stopTimer();
    }
}

void Spectrum_Display::timerCallback()
{
    update_plot_path (input_spectrum_path, input_spectrum);
    update_plot_path (output_spectrum_path, output_spectrum);
}

void Spectrum_Display::update_plot_path (juce::Path& path_to_update, Spectrum_Analyser& analyzer)
{
    path_to_update.clear();

    const juce::ScopedLock sl { analyzer.mutex };
    const auto& freq_axis = analyzer.fft_freqs;
    const auto& mag_response_db_smoothed = analyzer.fft_mags_smoothed_db;

    bool started = false;
    const auto n_points = freq_axis.size();
    for (size_t i = 0; i < n_points;)
    {
        if (freq_axis[i] < 20.0f || freq_axis[i] > 20'000.0f)
        {
            i++;
            continue;
        }

        if (! started)
        {
            auto x_draw = getXCoordinateForFrequency (freq_axis[i]);
            auto y_draw = getYCoordinateForDecibels (mag_response_db_smoothed[i]);
            path_to_update.startNewSubPath (x_draw, y_draw);
            started = true;
            i += 1;
        }
        else
        {
            if (i + 2 < n_points)
            {
                auto x_draw1 = getXCoordinateForFrequency (freq_axis[i]);
                auto y_draw1 = getYCoordinateForDecibels (mag_response_db_smoothed[i]);
                auto x_draw2 = getXCoordinateForFrequency (freq_axis[i + 1]);
                auto y_draw2 = getYCoordinateForDecibels (mag_response_db_smoothed[i + 1]);
                auto x_draw3 = getXCoordinateForFrequency (freq_axis[i + 2]);
                auto y_draw3 = getYCoordinateForDecibels (mag_response_db_smoothed[i + 2]);
                path_to_update.cubicTo ({ x_draw1, y_draw1 }, { x_draw2, y_draw2 }, { x_draw3, y_draw3 });
            }
            i += 3;
        }
    }

    path_to_update.lineTo (juce::Point { getWidth(), getHeight() }.toFloat());
    path_to_update.lineTo (juce::Point { 0, getHeight() }.toFloat());
    path_to_update.closeSubPath();

    repaint();
}
} // namespace viz
