#include "HotReloadedModule.h"

namespace
{
using namespace chowdsp::string_literals;
constexpr auto create_proc_tag = "create_processor"_sl;
constexpr auto delete_proc_tag = "delete_processor"_sl;
constexpr auto prepare_proc_tag = "prepare"_sl;
constexpr auto reset_proc_tag = "reset"_sl;
constexpr auto process_proc_tag = "process"_sl;

auto get_dll_source_path (const ModuleConfig& config)
{
    return juce::File { config.module_directory }.getChildFile ("main.cpp");
}

auto get_dll_build_dir_path (const ModuleConfig& config)
{
    return juce::File { config.module_directory }.getChildFile ("build");
}

auto get_dll_bin_path (const ModuleConfig& config)
{
#if JUCE_WINDOWS
    return get_dll_build_dir_path (config).getChildFile ("Debug/ProtoPlugTestModule.dll");
#elif JUCE_MAC
    return get_dll_build_dir_path (config).getChildFile ("Debug/libProtoPlugTestModule.dylib");
#endif
}

auto get_compile_command (const ModuleConfig& config)
{
    return juce::String { config.cmake_path } + " --build " + get_dll_build_dir_path (config).getFullPathName() + " --parallel";
}
} // namespace

HotReloadedModule::HotReloadedModule() = default;

HotReloadedModule::~HotReloadedModule()
{
    close_dll();
}

void HotReloadedModule::update_config (const ModuleConfig& new_config)
{
    config = new_config;
    file_watcher.emplace (get_dll_source_path (config));
    file_watcher->on_file_change = [this]
    { dll_source_file_changed(); };
}

void HotReloadedModule::dll_source_file_changed()
{
    juce::Logger::writeToLog ("Re-compiling module!");

    {
        juce::GenericScopedLock dll_lock { dll_reloading_mutex };
        close_dll();
    }

    juce::ChildProcess compiler {};
    compiler.start (get_compile_command (config));

    const auto start = std::chrono::steady_clock::now();
    const auto compiler_logs = compiler.readAllProcessOutput();
    const auto end = std::chrono::steady_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::duration<double>> (end - start);
    juce::Logger::writeToLog ("Compilation completed in " + juce::String { duration.count() } + " seconds");

    const auto exit_code = compiler.getExitCode();
    if (exit_code == 0)
    {
        load_dll();
    }
    else
    {
        juce::Logger::writeToLog ("Compiler failed with exit code: " + juce::String { exit_code });
        juce::Logger::writeToLog ("Compiler logs: " + compiler_logs);
    }
}

void HotReloadedModule::close_dll()
{
    if (processor_data[0] != nullptr)
    {
        for (auto* data : processor_data)
            destroy_proc_func (data);
    }
    create_proc_func = nullptr;
    destroy_proc_func = nullptr;
    prepare_proc_func = nullptr;
    reset_proc_func = nullptr;
    process_proc_func = nullptr;
    std::fill (processor_data.begin(), processor_data.end(), nullptr);
    dll.close();
}

void HotReloadedModule::load_dll()
{
    juce::GenericScopedLock dll_lock { dll_reloading_mutex };

    dll.open (get_dll_bin_path (config).getFullPathName());

    create_proc_func = reinterpret_cast<Create_Proc_Func> (dll.getFunction (create_proc_tag));
    destroy_proc_func = reinterpret_cast<Destroy_Proc_Func> (dll.getFunction (delete_proc_tag));
    prepare_proc_func = reinterpret_cast<Prepare_Proc_Func> (dll.getFunction (prepare_proc_tag));
    reset_proc_func = reinterpret_cast<Reset_Proc_Func> (dll.getFunction (reset_proc_tag));
    process_proc_func = reinterpret_cast<Process_Proc_Func> (dll.getFunction (process_proc_tag));

    if (create_proc_func == nullptr || destroy_proc_func == nullptr || prepare_proc_func == nullptr || reset_proc_func == nullptr
        || process_proc_func == nullptr)
    {
        juce::Logger::writeToLog ("Failed to load functions from DLL!");
        dll.close();
        return;
    }

    processor_data[0] = create_proc_func();
    processor_data[1] = create_proc_func();
    for (auto* data : processor_data)
        prepare_proc_func (data, process_spec.sampleRate, static_cast<int> (process_spec.maximumBlockSize));
}

void HotReloadedModule::prepare (const juce::dsp::ProcessSpec& spec)
{
    process_spec = spec;

    if (processor_data[0] != nullptr)
    {
        for (auto* data : processor_data)
            prepare_proc_func (data, process_spec.sampleRate, static_cast<int> (process_spec.maximumBlockSize));
    }
}

void HotReloadedModule::process (const chowdsp::BufferView<float>& buffer) noexcept
{
    juce::GenericScopedTryLock dll_try_lock { dll_reloading_mutex };
    if (! dll_try_lock.isLocked() || processor_data[0] == nullptr)
    {
        buffer.clear();
        return;
    }

    for (auto [ch, buffer_data] : chowdsp::buffer_iters::channels (buffer))
        process_proc_func (processor_data[(size_t) ch], buffer_data);

    if (! chowdsp::BufferMath::sanitizeBuffer (buffer, 10.0f))
    {
        for (auto* data : processor_data)
            reset_proc_func (data);
    }
}
