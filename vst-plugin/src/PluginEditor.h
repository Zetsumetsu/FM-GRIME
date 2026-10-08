#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

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
