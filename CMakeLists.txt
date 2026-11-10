cmake_minimum_required(VERSION 3.22)
# Embed the Microsoft C/C++ runtime so the VST3 does not depend on a
# redistributable installed on the developer / GitHub build machine.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
project(CurvePilotMidi01 VERSION 0.2.1 LANGUAGES C CXX)
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
  FORMATS VST3 Standalone PRODUCT_NAME "Curve Mini Prototype 0.2"
  IS_SYNTH TRUE NEEDS_MIDI_INPUT TRUE NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE COPY_PLUGIN_AFTER_BUILD FALSE VST3_CATEGORIES Instrument Synth)
juce_generate_juce_header(CurvePilotMidi01)
target_sources(CurvePilotMidi01 PRIVATE Source/Plugin.cpp Source/Editor.cpp Source/Plugin.h Source/MidiModel.h Source/MidiExport.h Source/Curve.h)
target_compile_definitions(CurvePilotMidi01 PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(CurvePilotMidi01 PRIVATE juce::juce_audio_utils PUBLIC juce::juce_recommended_config_flags)
if(MSVC)
  target_compile_options(CurvePilotMidi01 PRIVATE /utf-8)
endif()

# Exercise the actual processor/APVTS/state code, sharing the plugin's compiled
# JUCE modules. This adds no audio dependency to the model-only test mode.
if(BUILD_TESTING)
  add_executable(plugin_tests tests/plugin_tests.cpp)
  target_include_directories(plugin_tests PRIVATE Source $<TARGET_PROPERTY:CurvePilotMidi01,INCLUDE_DIRECTORIES>)
  target_link_libraries(plugin_tests PRIVATE CurvePilotMidi01)
  add_test(NAME plugin_state COMMAND plugin_tests)
endif()

# Independent host: loads the delivered DLL through the VST3 ABI. The processor
# tests above link shared code and cannot detect module-loading/packaging defects.
if(BUILD_TESTING AND WIN32)
  juce_add_console_app(vst3_load_tests PRODUCT_NAME "Curve Mini VST3 Load Test")
  juce_generate_juce_header(vst3_load_tests)
  target_sources(vst3_load_tests PRIVATE tests/vst3_load_tests.cpp)
  target_compile_definitions(vst3_load_tests PRIVATE JUCE_PLUGINHOST_VST3=1 JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
  target_link_libraries(vst3_load_tests PRIVATE juce::juce_audio_utils juce::juce_recommended_config_flags)
  add_dependencies(vst3_load_tests CurvePilotMidi01_VST3)
  add_test(NAME vst3_load COMMAND vst3_load_tests "$<TARGET_FILE_DIR:CurvePilotMidi01_VST3>/../..")
  set_tests_properties(vst3_load PROPERTIES TIMEOUT 90)
endif()
