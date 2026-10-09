#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

// ==================== FM GRIME GUI LAB ====================
// Tweak these values, push, and CI rebuilds the plugin.
// Colors are 0xAARRGGBB hex (AA = opacity, then red/green/blue).
//
// CRASH BISECT: if Ableton crashes, flip the USE_* switches off ONE at a
// time (true -> false), push, rebuild, test. The switch that stops the
// crash tells us exactly which feature Windows hates.
namespace GuiLab {
    // ---- window ----
    constexpr int WIN_W = 1020;
    constexpr int WIN_H = 470;

    // ---- colors ----
    inline const juce::Colour BG     = juce::Colour(0xff111111); // main background
    inline const juce::Colour STRIP  = juce::Colour(0xff1a1a1a); // voice strip panel
    inline const juce::Colour KNOB   = juce::Colour(0xff2b2b2b); // knob body
    inline const juce::Colour ACCENT = juce::Colour(0xffff3b1f); // red-orange accent
    inline const juce::Colour TEXT   = juce::Colour(0xffd8d8d8); // main text
    inline const juce::Colour DIM    = juce::Colour(0xff8a8a8a); // dim labels

    // ---- layout ----
    constexpr int STRIP_X0    = 10;   // first strip left edge
    constexpr int STRIP_Y     = 60;   // strip top edge
    constexpr int STRIP_PITCH = 130;  // horizontal distance between strips
    constexpr int STRIP_W     = 124;  // strip width
    constexpr int STRIP_H     = 396;  // strip height
    constexpr int KNOB_D      = 64;   // voice knob diameter
    constexpr int BIG_KNOB_D  = 88;   // degrade knob diameter
    constexpr int GLOBAL_X    = 806;  // global panel left edge
    constexpr int GLOBAL_W    = 204;  // global panel width

    // ---- crash-bisect switches ----
    constexpr bool USE_CUSTOM_KNOBS = true;  // custom knob rendering
    constexpr bool USE_ACTIVITY_TIMER = true; // 30 Hz LED flash timer
    constexpr bool USE_FANCY_MUTE   = true;  // rounded + glowing mute buttons
}

// Dark instrument look: charcoal panels, red-orange accents, glowing mutes.

class GrimeLookAndFeel : public juce::LookAndFeel_V4 {
public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override;
};

// Mute button doubling as the voice activity LED (mirrors the hardware:
// dark at rest, flashes with the trigger; dim red while muted).
class MuteButton : public juce::ToggleButton {
public:
    std::atomic<float> activity{0.0f};
    void paint(juce::Graphics& g) override;
};

// One voice strip: name, 3 knobs, mute/activity button.
class VoiceStrip : public juce::Component {
public:
    VoiceStrip(FmGrimeProcessor& proc, int voiceIndex, GrimeLookAndFeel& lnf);
    void resized() override;
    void setActivity(float a) { mute.activity.store(a); mute.repaint(); }

private:
    static void setupKnob(juce::Slider& s, GrimeLookAndFeel& lnf,
                          juce::Label& label, const juce::String& labelText);

    juce::Label nameLabel, pitchLabel, decayLabel, charLabel;
    juce::Slider pitchKnob, decayKnob, charKnob;
    MuteButton mute;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> aPitch, aDecay, aChar;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> aMute;
};

class PluginEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit PluginEditor(FmGrimeProcessor& proc);
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    static void setupKnob(juce::Slider& s, GrimeLookAndFeel& lnf,
                          juce::Label& label, const juce::String& labelText);

    FmGrimeProcessor& proc;
    GrimeLookAndFeel lnf;
    juce::Label titleLabel, globalLabel;
    std::unique_ptr<VoiceStrip> strips[6];

    juce::Label driveLabel, noiseLabel, masterLabel, degradeLabel;
    juce::Slider driveKnob, noiseKnob, masterKnob, degradeKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> aDrive, aNoise, aMaster, aDegrade;
};
