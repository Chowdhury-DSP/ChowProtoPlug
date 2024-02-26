#include "HotReloadedModule.h"

namespace
{
using namespace chowdsp::string_literals;
constexpr auto dll_source_dir = "/Users/jatin/ChowDSP/Plugins/ChowProtoPlug/TestModule/"_sl;
constexpr auto dll_source_path = dll_source_dir + "main.cpp"_sl;
constexpr auto dll_build_dir = dll_source_dir + "build/"_sl;
constexpr auto dll_path = dll_build_dir + "Debug/libProtoPlugTestModule.dylib"_sl;
constexpr auto create_proc_tag = "create_processor"_sl;
constexpr auto delete_proc_tag = "delete_processor"_sl;
constexpr auto prepare_proc_tag = "prepare"_sl;
constexpr auto reset_proc_tag = "reset"_sl;
constexpr auto process_proc_tag = "process"_sl;
} // namespace

HotReloadedModule::HotReloadedModule()
    : FileListener { juce::File { dll_source_path }, 1 }
{
    reload_dll();
}

HotReloadedModule::~HotReloadedModule()
{
    if (processor_data[0] != nullptr)
    {
        for (auto* data : processor_data)
            destroy_proc_func (data);
    }
}

void HotReloadedModule::listenerFileChanged()
{
    std::cout << "Re-compiling module!" << std::endl;

    juce::ChildProcess compiler {};
    compiler.start (juce::String { "/opt/homebrew/bin/cmake --build " } + juce::String { dll_build_dir } + " --parallel");
    // compiler.start ("echo \"This is a test!\"");

    const auto start = std::chrono::steady_clock::now();
    const auto compiler_logs = compiler.readAllProcessOutput();
    const auto end = std::chrono::steady_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::duration<double>> (end - start);
    std::cout << "Compilation completed in " << duration.count() << " seconds" << std::endl;

    const auto exit_code = compiler.getExitCode();
    if (exit_code == 0)
    {
        reload_dll();
    }
    else
    {
        std::cout << "Compiler failed with exit code: " << exit_code << std::endl;
        std::cout << "Compiler logs: " << compiler_logs << std::endl;
    }
}

void HotReloadedModule::reload_dll()
{
    juce::GenericScopedLock dll_lock { dll_reloading_mutex };

    if (processor_data[0] != nullptr)
    {
        for (auto*& data : processor_data)
            destroy_proc_func (data);
        std::fill (processor_data.begin(), processor_data.end(), nullptr);
    }

    dll.open (dll_path);

    create_proc_func = reinterpret_cast<Create_Proc_Func> (dll.getFunction (create_proc_tag));
    destroy_proc_func = reinterpret_cast<Destroy_Proc_Func> (dll.getFunction (delete_proc_tag));
    prepare_proc_func = reinterpret_cast<Prepare_Proc_Func> (dll.getFunction (prepare_proc_tag));
    reset_proc_func = reinterpret_cast<Reset_Proc_Func> (dll.getFunction (reset_proc_tag));
    process_proc_func = reinterpret_cast<Process_Proc_Func> (dll.getFunction (process_proc_tag));

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
        return;

    for (auto [ch, buffer_data] : chowdsp::buffer_iters::channels (buffer))
        process_proc_func (processor_data[(size_t) ch], buffer_data);

    if (! chowdsp::BufferMath::sanitizeBuffer (buffer, 10.0f))
    {
        for (auto* data : processor_data)
            reset_proc_func (data);
    }
}
