#include "HotReloadedModule.h"
#include <format>

namespace
{
using namespace chowdsp::string_literals;
constexpr auto create_proc_tag = "create_processor"_sl;
constexpr auto delete_proc_tag = "delete_processor"_sl;
constexpr auto prepare_proc_tag = "prepare"_sl;
constexpr auto reset_proc_tag = "reset"_sl;
constexpr auto process_proc_tag = "process"_sl;
constexpr auto get_num_float_params_tag = "get_num_float_params"_sl;
constexpr auto get_float_param_info_tag = "get_float_param_info"_sl;
constexpr auto set_float_param_tag = "set_float_param"_sl;
constexpr auto get_num_choice_params_tag = "get_num_choice_params"_sl;
constexpr auto get_choice_param_info_tag = "get_choice_param_info"_sl;
constexpr auto set_choice_param_tag = "set_choice_param"_sl;
constexpr std::string_view cmake_path { CMAKE_EXE_PATH };

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
    return get_dll_build_dir_path (config).getChildFile ("Debug/" + config.module_name + ".dll");
#elif JUCE_MAC
    return get_dll_build_dir_path (config).getChildFile ("Debug/lib" + config.module_name + ".dylib");
#endif
}

auto get_configure_command (const ModuleConfig& config)
{
    return chowdsp::toString (cmake_path)
           + " \"-DCMAKE_MAKE_PROGRAM=" + CMAKE_MAKE_PROGRAM + "\""
           + " \"-DCMAKE_C_COMPILER=" + CMAKE_C_COMPILER + "\""
           + " \"-DCMAKE_CXX_COMPILER=" + CMAKE_CXX_COMPILER + "\""
           + " -G \"" + CMAKE_GENERATOR + "\""
           + " -S \"" + juce::File { config.module_directory }.getFullPathName() + "\""
           + " -B \"" + get_dll_build_dir_path (config).getFullPathName() + "\"";
}

auto get_compile_command (const ModuleConfig& config)
{
    return chowdsp::toString (cmake_path) + " --build " + get_dll_build_dir_path (config).getFullPathName() + " --parallel";
}
} // namespace

HotReloadedModule::HotReloadedModule()
{
    old_cout_buffer = std::cout.rdbuf (&logging_buffer);
}

HotReloadedModule::~HotReloadedModule()
{
    close_dll();
    std::cout.rdbuf (old_cout_buffer);
}

void HotReloadedModule::update_config (const ModuleConfig& new_config)
{
    config = new_config;

    get_dll_build_dir_path (config).deleteRecursively();

    juce::ChildProcess configure {};
    configure.start (get_configure_command (config));
    const auto start = std::chrono::steady_clock::now();
    const auto compiler_logs = configure.readAllProcessOutput();
    const auto end = std::chrono::steady_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::duration<double>> (end - start);
    juce::Logger::writeToLog ("Configuration completed in " + juce::String { duration.count() } + " seconds");

    const auto exit_code = configure.getExitCode();
    if (exit_code != 0)
    {
        juce::Logger::writeToLog ("Configuration failed with exit code: " + juce::String { exit_code });
        juce::Logger::writeToLog ("Configuration logs: " + compiler_logs);
    }

    file_watcher.emplace (get_dll_source_path (config));
    file_watcher->on_file_change = [this]
    { dll_source_file_changed(); };
}

void HotReloadedModule::dll_source_file_changed()
{
    juce::Logger::writeToLog ("-----------------------------------------");
    juce::Logger::writeToLog ("Re-compiling module!");

    {
        juce::GenericScopedLock dll_lock { dll_reloading_mutex };
        close_dll();
    }

    if (! juce::File { chowdsp::toString (cmake_path) }.existsAsFile())
    {
        juce::Logger::writeToLog ("CMake executable not found! Path: " + chowdsp::toString (cmake_path));
        return;
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

void HotReloadedModule::load_dll()
{
    juce::GenericScopedLock dll_lock { dll_reloading_mutex };

    const auto module_path = get_dll_bin_path (config).getFullPathName();
    juce::Logger::writeToLog ("Loading module from path: " + module_path);
    dll.open (module_path);

    const auto func_table_loaded = load_function_table();
    if (! func_table_loaded)
    {
        juce::Logger::writeToLog ("Failed to load functions from DLL!");
        dll.close();
        return;
    }

    load_parameters();

    processor_data[0] = create_proc_func();
    processor_data[1] = create_proc_func();
    if (process_spec.sampleRate > 0.0)
    {
        for (auto* data : processor_data)
            prepare_proc_func (data, process_spec.sampleRate, static_cast<int> (process_spec.maximumBlockSize));
    }
}

bool HotReloadedModule::load_function_table()
{
    create_proc_func = reinterpret_cast<Create_Proc_Func> (dll.getFunction (create_proc_tag));
    destroy_proc_func = reinterpret_cast<Destroy_Proc_Func> (dll.getFunction (delete_proc_tag));
    prepare_proc_func = reinterpret_cast<Prepare_Proc_Func> (dll.getFunction (prepare_proc_tag));
    reset_proc_func = reinterpret_cast<Reset_Proc_Func> (dll.getFunction (reset_proc_tag));
    process_proc_func = reinterpret_cast<Process_Proc_Func> (dll.getFunction (process_proc_tag));
    get_num_float_params_func = reinterpret_cast<Get_Num_Float_Params_Func> (dll.getFunction (get_num_float_params_tag));
    get_float_param_info_func = reinterpret_cast<Get_Float_Param_Info_Func> (dll.getFunction (get_float_param_info_tag));
    set_float_param_func = reinterpret_cast<Set_Float_Param> (dll.getFunction (set_float_param_tag));
    get_num_choice_params_func = reinterpret_cast<Get_Num_Choice_Params_Func> (dll.getFunction (get_num_choice_params_tag));
    get_choice_param_info_func = reinterpret_cast<Get_Choice_Param_Info_Func> (dll.getFunction (get_choice_param_info_tag));
    set_choice_param_func = reinterpret_cast<Set_Choice_Param> (dll.getFunction (set_choice_param_tag));

    // These functions must be provided! All others are allowed to be nullptr.
    if (create_proc_func == nullptr || destroy_proc_func == nullptr || prepare_proc_func == nullptr || reset_proc_func == nullptr
        || process_proc_func == nullptr)
    {
        return false;
    }

    return true;
}

void HotReloadedModule::close_dll()
{
    params->clear_all_params();
    if (processor_data[0] != nullptr)
    {
        for (auto* data : processor_data)
            destroy_proc_func (data);
    }
    clear_function_table();
    std::fill (processor_data.begin(), processor_data.end(), nullptr);
    dll.close();
}

void HotReloadedModule::clear_function_table()
{
    create_proc_func = nullptr;
    destroy_proc_func = nullptr;
    prepare_proc_func = nullptr;
    reset_proc_func = nullptr;
    process_proc_func = nullptr;
    get_num_float_params_func = nullptr;
    get_float_param_info_func = nullptr;
    set_float_param_func = nullptr;
    get_num_choice_params_func = nullptr;
    get_choice_param_info_func = nullptr;
    set_choice_param_func = nullptr;
}

void HotReloadedModule::load_parameters() const
{
    using namespace chowdsp::ParamUtils;
    if (get_num_float_params_func != nullptr && get_float_param_info_func != nullptr)
    {
        const auto num_params = get_num_float_params_func();
        juce::Logger::writeToLog ("Module contains " + juce::String { num_params } + " float parameters!");
        for (int i = 0; i < num_params; ++i)
        {
            char name[128] {};
            float default_value, start, end, center;
            get_float_param_info_func (i, name, default_value, start, end, center);

            if (name[0] == '\0')
            {
                juce::Logger::writeToLog ("No param info provided for parameter index: " + std::to_string (i));
                continue;
            }

            juce::Logger::writeToLog ("Adding parameter: " + std::string { name }
                                      + ", {" + std::to_string (start) + "," + std::to_string (center) + "," + std::to_string (end) + "}"
                                      + ", default: " + std::to_string (default_value));
            params->float_params.emplace_back ("float_param" + std::to_string (i),
                                               name,
                                               createNormalisableRange (start, end, center),
                                               default_value,
                                               &floatValToStringDecimal<4>,
                                               &stringToFloatVal);
        }
    }

    if (get_num_choice_params_func != nullptr && get_choice_param_info_func != nullptr)
    {
        const auto num_params = get_num_choice_params_func();
        juce::Logger::writeToLog ("Module contains " + juce::String { num_params } + " choice parameters!");
        for (int i = 0; i < num_params; ++i)
        {
            char name[128] {};
            char choices[32][128] {};
            int default_value {};
            get_choice_param_info_func (i, name, choices, default_value);

            if (name[0] == '\0')
            {
                juce::Logger::writeToLog ("No param info provided for parameter index: " + std::to_string (i));
                continue;
            }

            juce::StringArray choices_array {};
            choices_array.ensureStorageAllocated (static_cast<int> (std::size (choices)));
            for (auto& choice : choices)
            {
                if (choice[0] == '\0')
                    break;
                choices_array.add (juce::String { choice });
            }

            std::stringstream ss {};
            std::copy (std::begin (choices_array),
                       std::end (choices_array),
                       std::ostream_iterator<juce::String> (ss, ", "));
            juce::Logger::writeToLog ("Adding parameter: " + std::string { name }
                                      + ", {" + ss.str() + "}"
                                      + ", default: " + choices[(size_t) default_value]);

            params->choice_params.emplace_back ("choice_param" + std::to_string (i),
                                                name,
                                                choices_array,
                                                default_value);
        }
    }

    params->finished_loading_params();
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
    {
        if (set_float_param_func != nullptr)
        {
            for (const auto [idx, param] : chowdsp::enumerate (params->float_params))
                set_float_param_func (processor_data[(size_t) ch], static_cast<int> (idx), param->getCurrentValue());
        }
        if (set_choice_param_func != nullptr)
        {
            for (const auto [idx, param] : chowdsp::enumerate (params->choice_params))
                set_choice_param_func (processor_data[(size_t) ch], static_cast<int> (idx), param->getIndex());
        }

        process_proc_func (processor_data[(size_t) ch], buffer_data);
    }

    if (! chowdsp::BufferMath::sanitizeBuffer (buffer, 10.0f))
    {
        for (auto* data : processor_data)
            reset_proc_func (data);
    }
}
