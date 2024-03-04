#!/bin/bash

# exit on failure
set -e

module_name="$1"
if [ -z "$module_name" ]; then
  echo "Usage: setup_module.sh <module_name>"
  exit 1
fi

#echo "Creating module ${module_name}..."
#
#mkdir -p proto/${module_name}
#
#cat > proto/${module_name}/CMakeLists.txt <<EOF
#cmake_minimum_required(VERSION 3.15)
#project(${module_name} VERSION 1.0.0)
#set(CMAKE_CXX_STANDARD 20)
#
#add_library(${module_name} SHARED main.cpp)
#
#if(MSVC)
#    target_compile_definitions(${module_name} PRIVATE _USE_MATH_DEFINES=1)
#endif()
#EOF
#
#cat > proto/${module_name}/main.cpp <<EOF
##include <iostream>
##include <span>
#
##if defined(WIN32)
##define DLL_EXPORT extern "C" __declspec(dllexport)
##else
##define DLL_EXPORT extern "C"
##endif
#
#DLL_EXPORT void test_function()
#{
#    std::cout << "Hello, World!" << std::endl;
#}
#
#struct Processor
#{
#    void prepare() {}
#    void reset() {}
#};
#
#DLL_EXPORT void* create_processor()
#{
#    std::cout << "Creating processor..." << std::endl;
#    return new Processor();
#}
#
#Processor* cast (void* processor)
#{
#    return static_cast<Processor*> (processor);
#}
#
#DLL_EXPORT void delete_processor (void* processor)
#{
#    std::cout << "Deleting processor..." << std::endl;
#    delete cast (processor);
#}
#
#DLL_EXPORT void prepare (void* processor, double sample_rate, int samples_per_block)
#{
#    std::cout << "Preparing processor with sample rate: " << sample_rate << ", and buffer size: " << samples_per_block << std::endl;
#    cast (processor)->prepare();
#}
#
#DLL_EXPORT void reset (void* processor) noexcept
#{
#    cast (processor)->reset();
#}
#
#DLL_EXPORT void process (void* processor, std::span<float> data)
#{
#    auto& proc = *cast (processor);
#
#    for (auto& x : data)
#        x *= 0.89f;
#}
#EOF
#
#echo "Setting up CMake for ${module_name}..."
#(
#    cd proto/${module_name}
#    if [[ "$OSTYPE" == "darwin"* ]]; then
#      cmake -Bbuild -G "Ninja Multi-Config"
#    elif [[ "$OSTYPE" == "msys" ]]; then
#      cmake -Bbuild -G "Ninja Multi-Config" -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl
#    fi
#    cmake --build build --config Debug --parallel
#)

echo "Updating proto settings..."
if [[ "$OSTYPE" == "darwin"* ]]; then
  config_file="$HOME/Library/ChowdhuryDSP/ChowProtoPlug/config.json"
  root_dir=$(pwd)
elif [[ "$OSTYPE" == "msys" ]]; then
  config_file="$HOME/AppData/Roaming/ChowdhuryDSP/ChowProtoPlug/config.json"
  root_dir=$(pwd -W)
fi

module_name_json=".module_name=\"${module_name}\""
module_dir_json=".module_directory=\"${root_dir}/proto/${module_name}\""
json_str=$(cat "${config_file}" | jq $module_name_json | jq $module_dir_json)
echo $json_str | jq .
rm "${config_file}"
echo $json_str | jq . > "${config_file}"
