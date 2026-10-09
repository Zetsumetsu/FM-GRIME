cmake_minimum_required(VERSION 3.22)
project(FMGRIME_VST VERSION 0.1.0)

# JUCE source: point JUCE_DIR at a local checkout, or let FetchContent grab it.
option(FMGRIME_FETCH_JUCE "Fetch JUCE via CMake FetchContent" ON)
set(JUCE_DIR "" CACHE PATH "Path to a local JUCE checkout (used when FMGRIME_FETCH_JUCE is OFF)")

if(FMGRIME_FETCH_JUCE)
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.6
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
else()
    add_subdirectory(${JUCE_DIR} JUCE)
endif()

juce_add_plugin(FmGrimeVST
    COMPANY_NAME "Zetsumetsu"
    PLUGIN_NAME "FM GRIME"
    PRODUCT_NAME "FM GRIME"
    VERSION "0.1.0"
    FORMATS VST3
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    PLUGIN_MANUFACTURER_CODE Zets
    PLUGIN_CODE Fgrm
    VST3_CATEGORIES "Instrument" "Drums"
    # Add AU on macOS builds: FORMATS VST3 AU
)

target_sources(FmGrimeVST PRIVATE src/PluginProcessor.cpp src/PluginEditor.cpp)

# Embedded GUI artwork (slime background + title banner) via JUCE BinaryData.
# BinaryData.h exposes BinaryData::slime_bg_jpg / grime_title_png (+Size).
juce_add_binary_data(FmGrimeAssets SOURCES
    assets/slime_bg.jpg
    assets/grime_title.png)

target_compile_definitions(FmGrimeVST PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(FmGrimeVST PRIVATE
    FmGrimeAssets
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags)

# Headless smoke test (loads the built .vst3, fires MIDI, checks audio).
# Dev tooling only; not part of the plugin.
option(FMGRIME_BUILD_SMOKE_TEST "Build the headless plugin smoke test" OFF)
if(FMGRIME_BUILD_SMOKE_TEST)
    add_subdirectory(tools/smoke_test)
endif()
