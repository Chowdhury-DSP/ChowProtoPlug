function(setup_source_group src_group_name)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(ARG "" "" "${multiValueArgs}" ${ARGN})

    message(STATUS "Setting up source group ${src_group_name}, from SOURCES: ${ARG_SOURCES}")
    target_sources(ChowProtoPlug PRIVATE ${ARG_SOURCES})
    set_source_files_properties(
        ${ARG_SOURCES}
        TARGET_DIRECTORY ChowProtoPlug
        PROPERTIES UNITY_GROUP "${src_group_name}"
    )
endfunction(setup_source_group)

setup_source_group(SRCS SOURCES
    ChowProtoPlug.cpp
    HotReloadedModule.cpp
    ModuleParams.cpp
    PluginEditor.cpp

    visualizers/Oscilloscope.cpp
    visualizers/SpectrumAnalyzer.cpp
)

file(GLOB_RECURSE juce_module_sources CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../modules/JUCE/modules/juce_*/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../modules/JUCE/modules/juce_*/*.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/../modules/JUCE/modules/chowdsp_utils/modules/*/chowdsp_*/*.cpp
)
set_source_files_properties(${juce_module_sources}
    TARGET_DIRECTORY ChowProtoPlug
    PROPERTIES SKIP_PRECOMPILE_HEADERS TRUE SKIP_UNITY_BUILD_INCLUSION TRUE
)
set_target_properties(ChowProtoPlug PROPERTIES
    UNITY_BUILD ON
    UNITY_BUILD_MODE GROUP
    UNITY_BUILD_BATCH_SIZE 8
)
target_precompile_headers(ChowProtoPlug PRIVATE pch.h)
