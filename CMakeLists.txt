# =====================================================================
# Berlin Vocal AI - Phase 1
# CMake build script (JUCE 8, VST3 only, Windows x64 / Linux x64)
#
# JUCE is fetched at configure time with a pinned release tag so the
# build is reproducible from a clean checkout. Tradeoff: the first
# configure requires an internet connection. To vendor JUCE manually,
# download JUCE 8.0.15 and set JUCE_PATH to its folder before
# configuring (see docs/BUILD_WINDOWS.md).
# =====================================================================
cmake_minimum_required(VERSION 3.22)

project(BerlinVocalAI VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Keep build outputs out of the source tree when using presets/scripts
# (users may still pass their own -B path; nothing is written inside
# the source directory by this file itself).

option(BVA_BUILD_TESTS "Build the headless DSP/parameter test runner" ON)

# ---------------------------------------------------------------------
# JUCE via FetchContent (pinned tag)
# ---------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.15
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# ---------------------------------------------------------------------
# Plugin target (VST3 only in Phase 1 - no Standalone/AU/AAX)
# ---------------------------------------------------------------------
juce_add_plugin(BerlinVocalAI
    COMPANY_NAME                "Berlin Vocal AI"
    BUNDLE_ID                   com.berlinvocalai.berlinvocalai
    PLUGIN_MANUFACTURER_CODE    Bvai
    PLUGIN_CODE                 Bva1
    FORMATS                     VST3
    PRODUCT_NAME                "Berlin Vocal AI"
    VST3_CATEGORIES             Fx
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
)

target_sources(BerlinVocalAI PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Parameters.cpp
    Source/dsp/VocalProcessor.cpp
    Source/dsp/ThreeBandEQ.cpp
    Source/dsp/VocalCompressor.cpp
    Source/dsp/DeEsser.cpp
)

target_compile_definitions(BerlinVocalAI PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(BerlinVocalAI
    PRIVATE
        juce::juce_audio_processors
        juce::juce_gui_basics
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ---------------------------------------------------------------------
# Headless test runner (no GUI required): links the real processor and
# DSP code and exercises it with deterministic synthetic signals.
# ---------------------------------------------------------------------
if(BVA_BUILD_TESTS)
    enable_testing()

    juce_add_console_app(bva_tests PRODUCT_NAME "bva_tests")

    target_sources(bva_tests PRIVATE
        Tests/TestMain.cpp
        Tests/DSPTests.cpp
        Tests/ParameterTests.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Parameters.cpp
        Source/dsp/VocalProcessor.cpp
        Source/dsp/ThreeBandEQ.cpp
        Source/dsp/VocalCompressor.cpp
        Source/dsp/DeEsser.cpp
    )

    target_compile_definitions(bva_tests PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        BVA_HEADLESS_TESTS=1
    )

    target_link_libraries(bva_tests
        PRIVATE
            juce::juce_audio_processors
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
    )
endif()
