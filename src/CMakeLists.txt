target_sources(ChowProtoPlug PRIVATE
    ChowProtoPlug.cpp
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
target_precompile_headers(ChowProtoPlug PRIVATE pch.h)
    