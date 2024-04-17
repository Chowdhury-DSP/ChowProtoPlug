#pragma once

#include "ModuleConfig.h"
#include "ModuleParams.h"

struct HotReloadedModule
{
    HotReloadedModule();

    ~HotReloadedModule();

    void update_config (const ModuleConfig& config);

    void prepare (const juce::dsp::ProcessSpec&);

    void process (const chowdsp::BufferView<float>&) noexcept;

    void run_cmake_configure();
    void dll_source_file_changed();

    void load_dll();
    void close_dll();

    bool load_function_table();
    void clear_function_table();

    ModuleParams* params = nullptr;

    void load_parameters() const;

    struct FileWatcher : chowdsp::FileListener
    {
        explicit FileWatcher (const juce::File& file) : chowdsp::FileListener (file, 1)
        {
        }

        std::function<void()> on_file_change {};

        void listenerFileChanged() override
        {
            if (on_file_change)
                on_file_change();
        }
    };

    ModuleConfig config;
    std::optional<FileWatcher> file_watcher;

    using Create_Proc_Func = void* (*) ();
    Create_Proc_Func create_proc_func = nullptr;
    using Destroy_Proc_Func = void (*) (void*);
    Destroy_Proc_Func destroy_proc_func = nullptr;

    using Prepare_Proc_Func = void (*) (void*, double, int);
    Prepare_Proc_Func prepare_proc_func = nullptr;
    using Reset_Proc_Func = void (*) (void*);
    Reset_Proc_Func reset_proc_func = nullptr;
    using Process_Proc_Func = void (*) (void*, std::span<float>);
    Process_Proc_Func process_proc_func = nullptr;

    using Get_Num_Float_Params_Func = int (*)();
    Get_Num_Float_Params_Func get_num_float_params_func = nullptr;
    using Get_Float_Param_Info_Func = void (*) (int param_index, char (&name)[128], float& default_value, float& start, float& end, float& center);
    Get_Float_Param_Info_Func get_float_param_info_func = nullptr;
    using Set_Float_Param = void (*) (void*, int, float);
    Set_Float_Param set_float_param_func = nullptr;

    using Get_Num_Choice_Params_Func = int (*)();
    Get_Num_Choice_Params_Func get_num_choice_params_func = nullptr;
    using Get_Choice_Param_Info_Func = void (*) (int param_index, char (&name)[128], char (&choices)[32][128], int& default_value);
    Get_Choice_Param_Info_Func get_choice_param_info_func = nullptr;
    using Set_Choice_Param = void (*) (void*, int index, int value);
    Set_Choice_Param set_choice_param_func = nullptr;

    juce::dsp::ProcessSpec process_spec {};
    juce::DynamicLibrary dll {};
    juce::SpinLock dll_reloading_mutex {};
    std::array<void*, 2> processor_data {};

    struct LoggingBuffer : std::stringbuf
    {
        int sync() override
        {
            auto current_str = this->str();
            if (current_str.back() == '\n')
                current_str = current_str.substr (0, current_str.size() - 1);
            juce::Logger::writeToLog ("[module_log] " + current_str);
            this->str ("");
            return 0;
        }
    } logging_buffer;

    std::streambuf* old_cout_buffer = nullptr;
};
