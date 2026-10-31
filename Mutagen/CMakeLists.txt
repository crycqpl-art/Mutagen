cmake_minimum_required(VERSION 3.22)

project(Mutagen VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: use a local copy if MUTAGEN_JUCE_DIR is given, otherwise download it.
set(MUTAGEN_JUCE_DIR "" CACHE PATH "Path to a local JUCE checkout (optional)")
option(MUTAGEN_BUILD_TESTS "Build the offline test program" OFF)

if(MUTAGEN_JUCE_DIR)
    add_subdirectory(${MUTAGEN_JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.10
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

set(MUTAGEN_FORMATS VST3)
if(APPLE)
    list(APPEND MUTAGEN_FORMATS AU)
endif()

juce_add_plugin(Mutagen
    COMPANY_NAME "Kamil Potebski"
    COMPANY_WEBSITE ""
    BUNDLE_ID "com.kamilpotebski.mutagen"
    PLUGIN_MANUFACTURER_CODE Kpot
    PLUGIN_CODE Mtgn
    FORMATS ${MUTAGEN_FORMATS}
    PRODUCT_NAME "Mutagen"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx
    AU_MAIN_TYPE kAudioUnitType_Effect)

set(MUTAGEN_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Presets.cpp)

target_sources(Mutagen PRIVATE ${MUTAGEN_SOURCES})

set(MUTAGEN_DEFINES
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

if(UNIX AND NOT APPLE)
    # Lets the plugin build on minimal Linux machines (only used for testing).
    list(APPEND MUTAGEN_DEFINES JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0)
endif()

target_compile_definitions(Mutagen PUBLIC ${MUTAGEN_DEFINES})

target_link_libraries(Mutagen
    PRIVATE
        juce::juce_audio_processors
        juce::juce_audio_formats
        juce::juce_dsp
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(MUTAGEN_BUILD_TESTS)
    juce_add_console_app(MutagenTest PRODUCT_NAME "MutagenTest")
    target_sources(MutagenTest PRIVATE Tests/OfflineTest.cpp ${MUTAGEN_SOURCES})
    target_compile_definitions(MutagenTest PRIVATE ${MUTAGEN_DEFINES}
        JucePlugin_Name="Mutagen" JucePlugin_IsSynth=0 JucePlugin_WantsMidiInput=0
        JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0)
    target_link_libraries(MutagenTest PRIVATE
        juce::juce_audio_processors juce::juce_audio_formats juce::juce_dsp juce::juce_gui_basics
        juce::juce_recommended_config_flags)
endif()
