#pragma once

#include "ModuleConfig.h"

struct HotReloadedModule
{
    explicit HotReloadedModule (const ModuleConfig& config);
    ~HotReloadedModule();

    void update_config (const ModuleConfig& config);

    void prepare (const juce::dsp::ProcessSpec&);
    void process (const chowdsp::BufferView<float>&) noexcept;

    void dll_source_file_changed();
    void close_dll();
    void load_dll();

    struct FileWatcher : chowdsp::FileListener
    {
        explicit FileWatcher (const juce::File& file) : chowdsp::FileListener (file, 1) {}

        std::function<void()> on_file_change {};
        void listenerFileChanged() override
        {
            if (on_file_change)
                on_file_change();
        }
    };

    ModuleConfig config;
    std::optional<FileWatcher> file_watcher;

    using Create_Proc_Func = void* (*)();
    Create_Proc_Func create_proc_func = nullptr;
    using Destroy_Proc_Func = void (*)(void*);
    Destroy_Proc_Func destroy_proc_func = nullptr;
    using Prepare_Proc_Func = void (*)(void*, double, int);
    Prepare_Proc_Func prepare_proc_func = nullptr;
    using Reset_Proc_Func = void (*)(void*);
    Reset_Proc_Func reset_proc_func = nullptr;
    using Process_Proc_Func = void (*)(void*, std::span<float>);
    Process_Proc_Func process_proc_func = nullptr;

    juce::dsp::ProcessSpec process_spec {};
    juce::DynamicLibrary dll {};
    juce::SpinLock dll_reloading_mutex {};
    std::array<void*, 2> processor_data {};
};
