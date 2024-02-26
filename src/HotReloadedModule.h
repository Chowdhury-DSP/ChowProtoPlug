#pragma once

#include <pch.h>

struct HotReloadedModule : chowdsp::FileListener
{
    HotReloadedModule();
    ~HotReloadedModule() override;

    void prepare (const juce::dsp::ProcessSpec&);
    void process (const chowdsp::BufferView<float>&) noexcept;

    void listenerFileChanged() override;
    void reload_dll();

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
