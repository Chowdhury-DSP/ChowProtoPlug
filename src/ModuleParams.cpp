#include "ModuleParams.h"

ModuleParams::ModuleParams()
    : ParamHolder { "Module Params", false }
{
}

void ModuleParams::clear_all_params()
{
    params_cleared();
    forwarding_params->clearParameterRange (0, num_forward_parameters);
    clear();
    float_params.clear();
    choice_params.clear();
    param_listeners.emplace (*this);
}

void ModuleParams::finished_loading_params()
{
    add (float_params, choice_params);
    forwarding_params->setParameterRange (0,
                                          std::min (static_cast<int> (float_params.size() + choice_params.size()),
                                                    num_forward_parameters),
                                          [this] (int idx) -> chowdsp::ParameterForwardingInfo
                                          {
                                              auto index = static_cast<size_t> (idx);
                                              if (index < float_params.size())
                                              {
                                                  auto* param = float_params[index].get();
                                                  return { param, param->name };
                                              }

                                              index -= float_params.size();
                                              if (index < choice_params.size())
                                              {
                                                  auto* param = choice_params[index].get();
                                                  return { param, param->name };
                                              }

                                              return {};
                                          });
    param_listeners.emplace (*this);
    params_added();
}
