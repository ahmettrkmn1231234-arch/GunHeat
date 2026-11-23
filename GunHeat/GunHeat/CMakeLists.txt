cmake_minimum_required(VERSION 3.22)
project(GunHeat VERSION 1.0.0)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(GunHeat
    COMPANY_NAME "MyStudio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE Mstd
    PLUGIN_CODE Gnht
    FORMATS VST3
    PRODUCT_NAME "Gun Heat"
    COPY_PLUGIN_AFTER_BUILD TRUE)

target_sources(GunHeat PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(GunHeat PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(GunHeat
    PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
