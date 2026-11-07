cmake_minimum_required(VERSION 3.22)
project(CurvePilotMidi01 VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(CTest)
add_executable(midi_tests tests/midi_tests.cpp)
add_test(NAME midi_model COMMAND midi_tests)
option(MIDI_TESTS_ONLY "Only test the MIDI curve model" OFF)
if(MIDI_TESTS_ONLY)
  return()
endif()
include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.6 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)
# A separate silent control instrument track. Direct OS MIDI output avoids
# VST3 CC-output limitations and reaches Ableton's Remote input via loopMIDI.
juce_add_plugin(CurvePilotMidi01 COMPANY_NAME ANKS PLUGIN_MANUFACTURER_CODE Anks PLUGIN_CODE Cpm1
  FORMATS VST3 Standalone PRODUCT_NAME "Curve Mini Prototype 0.1"
  IS_SYNTH TRUE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE COPY_PLUGIN_AFTER_BUILD FALSE VST3_CATEGORIES Instrument)
juce_generate_juce_header(CurvePilotMidi01)
target_sources(CurvePilotMidi01 PRIVATE Source/Plugin.cpp Source/Editor.cpp Source/Plugin.h Source/MidiModel.h Source/Curve.h)
target_compile_definitions(CurvePilotMidi01 PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(CurvePilotMidi01 PRIVATE juce::juce_audio_utils PUBLIC juce::juce_recommended_config_flags)
if(MSVC)
  target_compile_options(CurvePilotMidi01 PRIVATE /utf-8)
endif()
