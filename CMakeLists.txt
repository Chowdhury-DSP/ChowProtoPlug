cmake_minimum_required(VERSION 3.15)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment target")
project(ChowProtoPlug VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(COMPANY_NAME "chowdsp")
set(PRODUCT_NAME "ChowProtoPlug")

add_subdirectory(modules)

set(JUCE_FORMATS AU VST3 Standalone)

juce_add_plugin(ChowProtoPlug
    COMPANY_NAME "${COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE Chow
    PLUGIN_CODE Cpp4
    FORMATS ${JUCE_FORMATS}
    PRODUCT_NAME "${PRODUCT_NAME}"
    
    MICROPHONE_PERMISSION_ENABLED TRUE
)

if(NOT IOS)
    clap_juce_extensions_plugin(
        TARGET ChowProtoPlug
        CLAP_ID "org.chowdsp.ChowProtoPlug"
        CLAP_FEATURES audio-effect
        CLAP_PROCESS_EVENTS_RESOLUTION_SAMPLES 64
        CLAP_USE_JUCE_PARAMETER_RANGES DISCRETE
    )
endif()

add_subdirectory(src)
include_directories(src)

target_compile_definitions(ChowProtoPlug PUBLIC
    JUCE_VST3_CAN_REPLACE_VST2=0
    DEFAULT_MODULE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/TestModule"
    CMAKE_EXE_PATH="${CMAKE_COMMAND}"
    CMAKE_GENERATOR="${CMAKE_GENERATOR}"
    CMAKE_MAKE_PROGRAM="${CMAKE_MAKE_PROGRAM}"
    CMAKE_C_COMPILER="${CMAKE_C_COMPILER}"
    CMAKE_CXX_COMPILER="${CMAKE_CXX_COMPILER}"
)

target_link_libraries(ChowProtoPlug PRIVATE juce_plugin_modules)
