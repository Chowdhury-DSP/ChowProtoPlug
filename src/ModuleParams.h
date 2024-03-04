#pragma once

#include <pch.h>

struct ModuleParams : chowdsp::ParamHolder
{
    struct ParamForwardingProvider
    {
        static chowdsp::ParameterID getForwardingParameterID (int paramIndex)
        {
            return { "forward_param_" + juce::String (paramIndex), 100 };
        }
    };

    static constexpr int num_forward_parameters = 20;
    using ForwardingParams = chowdsp::ForwardingParametersManager<ParamForwardingProvider, num_forward_parameters>;
    std::optional<ForwardingParams> forwarding_params;

    std::vector<chowdsp::FloatParameter::Ptr> float_params;
    std::vector<chowdsp::ChoiceParameter::Ptr> choice_params;

    std::optional<chowdsp::ParameterListeners> param_listeners { std::nullopt };
    chowdsp::Broadcaster<void()> params_cleared {};
    chowdsp::Broadcaster<void()> params_added {};

    ModuleParams();
    void clear_all_params();
    void finished_loading_params();
};
