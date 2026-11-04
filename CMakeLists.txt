cmake_minimum_required(VERSION 3.22)
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
project(UNDERTOW VERSION 0.8.0)
set(CMAKE_CXX_STANDARD 20)
# A JUCE 8 checkout: ./JUCE if there is one, otherwise NEON RIOT's when the two
# projects sit side by side. Or pass -DJUCE_DIR=...
if(EXISTS "${CMAKE_SOURCE_DIR}/JUCE/CMakeLists.txt")
    set(UNDERTOW_JUCE "${CMAKE_SOURCE_DIR}/JUCE")
else()
    set(UNDERTOW_JUCE "${CMAKE_SOURCE_DIR}/../NEON RIOT/JUCE")
endif()
set(JUCE_DIR "${UNDERTOW_JUCE}" CACHE PATH "Path to a JUCE 8 checkout")
add_subdirectory("${JUCE_DIR}" JUCE)
juce_add_plugin(Undertow COMPANY_NAME "Rob Cawley" COMPANY_WEBSITE "https://robcawley.com"
    BUNDLE_ID com.robcawley.undertow PLUGIN_MANUFACTURER_CODE Rcwl PLUGIN_CODE Utow
    PRODUCT_NAME "UNDERTOW" DESCRIPTION "Ambient space effect" IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE IS_MIDI_EFFECT FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect FORMATS AU COPY_PLUGIN_AFTER_BUILD TRUE VERSION 0.8.0)
juce_add_binary_data(UndertowAssets SOURCES
    Resources/Fonts/Rajdhani-SemiBold.ttf
    Resources/Fonts/Rajdhani-Bold.ttf
    Resources/Fonts/Rajdhani-Medium.ttf
    Resources/Fonts/ShareTechMono-Regular.ttf)
set(CORE Source/Core/Specs.cpp Source/Core/Engine.cpp Source/Core/Pulse.cpp Source/Core/Modulation.cpp
    Source/Core/Presets.cpp)
set(ENGINE ${CORE} Source/Parameters.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/UI/Theme.cpp Source/UI/Chassis.cpp Source/UI/Controls.cpp
    Source/UI/Tooltips.cpp Source/UI/Displays.cpp)
target_sources(Undertow PRIVATE ${ENGINE})
target_include_directories(Undertow PRIVATE Source)
target_compile_definitions(Undertow PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(Undertow PRIVATE UndertowAssets juce::juce_audio_utils juce::juce_dsp PUBLIC juce::juce_recommended_config_flags)
juce_add_console_app(UndertowRender PRODUCT_NAME UndertowRender)
target_sources(UndertowRender PRIVATE Tools/Render.cpp ${ENGINE})
target_include_directories(UndertowRender PRIVATE Source)
target_compile_definitions(UndertowRender PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(UndertowRender PRIVATE UndertowAssets juce::juce_audio_utils juce::juce_audio_formats juce::juce_dsp juce::juce_recommended_config_flags)
juce_add_console_app(UndertowHostTest PRODUCT_NAME UndertowHostTest)
target_sources(UndertowHostTest PRIVATE Tools/HostTest.cpp)
target_compile_definitions(UndertowHostTest PRIVATE JUCE_PLUGINHOST_AU=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(UndertowHostTest PRIVATE juce::juce_audio_utils juce::juce_recommended_config_flags)
juce_add_console_app(UndertowPairs PRODUCT_NAME UndertowPairs)
target_sources(UndertowPairs PRIVATE Tools/Pairs.cpp)
target_compile_definitions(UndertowPairs PRIVATE JUCE_PLUGINHOST_AU=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(UndertowPairs PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)

# The core on its own, with no JUCE anywhere on the include path: proves the
# MIT core stands alone.
add_executable(UndertowCoreCheck Tools/CoreCheck.cpp ${CORE})
target_include_directories(UndertowCoreCheck PRIVATE Source/Core)

# The game-facing core: Processor, game presets and the C interface.
set(GAME_CORE ${CORE} Source/Core/Processor.cpp Source/Core/GamePresets.cpp Source/Core/undertow.cpp)
add_executable(UndertowGameCheck Tools/GameCheck.cpp ${GAME_CORE})
target_include_directories(UndertowGameCheck PRIVATE Source/Core)
target_compile_options(UndertowGameCheck PRIVATE -O3)
