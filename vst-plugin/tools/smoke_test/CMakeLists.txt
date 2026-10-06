juce_add_console_app(FmGrimeSmokeTest)
target_sources(FmGrimeSmokeTest PRIVATE Main.cpp)
target_compile_definitions(FmGrimeSmokeTest PRIVATE
    JUCE_PLUGINHOST_VST3=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)
target_link_libraries(FmGrimeSmokeTest PRIVATE juce::juce_audio_processors)
