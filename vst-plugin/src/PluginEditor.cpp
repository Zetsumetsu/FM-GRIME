#include "PluginEditor.h"

namespace {
constexpr float kPi = 3.14159265358979323846f;
const juce::Colour kBg(0xff111111);
const juce::Colour kStrip(0xff1a1a1a);
const juce::Colour kKnob(0xff2b2b2b);
const juce::Colour kKnobEdge(0xff454545);
const juce::Colour kAccent(0xffff3b1f);
const juce::Colour kText(0xffd8d8d8);
const juce::Colour kDim(0xff8a8a8a);

const char* kVoiceNames[6] = {"KICK", "SNARE", "HAT CL", "HAT OP", "PERC", "WILD"};
const char* kVoiceIds[6] = {"kick", "snare", "hatcl", "hatop", "perc", "wild"};
}  // namespace

// ---- GrimeLookAndFeel ----

void GrimeLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float rotaryStartAngle,
                                        float rotaryEndAngle, juce::Slider&) {
    auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height)
                      .reduced(4.0f);
    float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    auto centre = bounds.getCentre();

    g.setColour(kKnob);
    g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
    g.setColour(kKnobEdge);
    g.drawEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, 1.5f);

    float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    float arcR = radius - 5.0f;
    if (sliderPos > 0.001f) {
        juce::Path arc;
        arc.addArc(centre.x - arcR, centre.y - arcR, arcR * 2.0f, arcR * 2.0f,
                   rotaryStartAngle, angle, true);
        g.setColour(kAccent);
        g.strokePath(arc, juce::PathStrokeType(3.0f));
    }

    float pointerLen = radius * 0.72f;
    juce::Path pointer;
    pointer.addLineSegment(
        juce::Line<float>(centre, centre.getPointOnCircumference(pointerLen, angle)), 3.0f);
    g.setColour(juce::Colours::white);
    g.strokePath(pointer, juce::PathStrokeType(3.0f, juce::PathStrokeType::mitered,
                                               juce::PathStrokeType::rounded));
}

// ---- MuteButton ----

void MuteButton::paint(juce::Graphics& g) {
    auto b = getLocalBounds().toFloat().reduced(2.0f);
    float act = juce::jmin(1.0f, activity.load());
    bool muted = getToggleState();

    g.setColour(muted ? juce::Colour(0xff5a1408) : juce::Colour(0xff242424));
    g.fillRoundedRectangle(b, 6.0f);

    if (act > 0.01f) {
        g.setColour(kAccent.withAlpha(act * (muted ? 0.95f : 0.6f)));
        g.fillRoundedRectangle(b, 6.0f);
    }

    g.setColour(muted ? juce::Colours::white : kDim);
    g.setFont(12.0f);
    g.drawText(muted ? "MUTED" : "MUTE", b, juce::Justification::centred);
}

// ---- VoiceStrip ----

void VoiceStrip::setupKnob(juce::Slider& s, GrimeLookAndFeel& lnf,
                           juce::Label& label, const juce::String& labelText) {
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setRotaryParameters(kPi * 1.25f, kPi * 2.75f, true);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    s.setLookAndFeel(&lnf);
    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, kDim);
    label.setFont(11.0f);
}

VoiceStrip::VoiceStrip(FmGrimeProcessor& proc, int voiceIndex, GrimeLookAndFeel& lnf) {
    setupKnob(pitchKnob, lnf, pitchLabel, "PITCH");
    setupKnob(decayKnob, lnf, decayLabel, "DECAY");
    setupKnob(charKnob, lnf, charLabel, "CHAR");

    nameLabel.setText(kVoiceNames[voiceIndex], juce::dontSendNotification);
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setColour(juce::Label::textColourId, kText);
    nameLabel.setFont(juce::Font(14.0f, juce::Font::bold));

    addAndMakeVisible(pitchKnob);
    addAndMakeVisible(decayKnob);
    addAndMakeVisible(charKnob);
    addAndMakeVisible(mute);
    addAndMakeVisible(nameLabel);
    addAndMakeVisible(pitchLabel);
    addAndMakeVisible(decayLabel);
    addAndMakeVisible(charLabel);

    juce::String vid = kVoiceIds[voiceIndex];
    aPitch = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, vid + "_pitch", pitchKnob);
    aDecay = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, vid + "_decay", decayKnob);
    aChar = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, vid + "_char", charKnob);
    aMute = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, vid + "_mute", mute);
}

void VoiceStrip::resized() {
    auto b = getLocalBounds().reduced(4);
    nameLabel.setBounds(b.removeFromTop(24));
    auto knobRow = [&](juce::Slider& s, juce::Label& l) {
        auto row = b.removeFromTop(78);
        l.setBounds(row.removeFromBottom(16));
        s.setBounds(row);
    };
    knobRow(pitchKnob, pitchLabel);
    knobRow(decayKnob, decayLabel);
    knobRow(charKnob, charLabel);
    b.removeFromTop(8);
    mute.setBounds(b.removeFromTop(40));
}

// ---- PluginEditor ----

void PluginEditor::setupKnob(juce::Slider& s, GrimeLookAndFeel& lnf,
                             juce::Label& label, const juce::String& labelText) {
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setRotaryParameters(kPi * 1.25f, kPi * 2.75f, true);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    s.setLookAndFeel(&lnf);
    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, kDim);
    label.setFont(11.0f);
}

PluginEditor::PluginEditor(FmGrimeProcessor& p)
    : juce::AudioProcessorEditor(p), proc(p) {
    setSize(1020, 470);

    titleLabel.setText("FM GRIME", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setColour(juce::Label::textColourId, kText);
    titleLabel.setFont(juce::Font(26.0f, juce::Font::bold));
    addAndMakeVisible(titleLabel);

    globalLabel.setText("GLOBAL", juce::dontSendNotification);
    globalLabel.setJustificationType(juce::Justification::centred);
    globalLabel.setColour(juce::Label::textColourId, kText);
    globalLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    addAndMakeVisible(globalLabel);

    for (int i = 0; i < 6; i++) {
        strips[i] = std::make_unique<VoiceStrip>(p, i, lnf);
        addAndMakeVisible(strips[i].get());
    }

    setupKnob(driveKnob, lnf, driveLabel, "DRIVE");
    setupKnob(noiseKnob, lnf, noiseLabel, "NOISE");
    setupKnob(masterKnob, lnf, masterLabel, "MASTER");
    setupKnob(degradeKnob, lnf, degradeLabel, "DEGRADE");
    addAndMakeVisible(driveKnob);
    addAndMakeVisible(noiseKnob);
    addAndMakeVisible(masterKnob);
    addAndMakeVisible(degradeKnob);
    addAndMakeVisible(driveLabel);
    addAndMakeVisible(noiseLabel);
    addAndMakeVisible(masterLabel);
    addAndMakeVisible(degradeLabel);

    aDrive = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "drive", driveKnob);
    aNoise = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "noise", noiseKnob);
    aMaster = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "master", masterKnob);
    aDegrade = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "degrade", degradeKnob);

    startTimerHz(30);
}

void PluginEditor::paint(juce::Graphics& g) {
    g.fillAll(kBg);
    // strip backgrounds
    for (int i = 0; i < 6; i++) {
        auto b = strips[i]->getBounds().toFloat();
        g.setColour(kStrip);
        g.fillRoundedRectangle(b, 8.0f);
    }
    auto gb = juce::Rectangle<float>(806, 56, 204, 404);
    g.setColour(kStrip);
    g.fillRoundedRectangle(gb, 8.0f);
    // red rule under title
    g.setColour(kAccent);
    g.fillRect(10, 50, 1000, 2);
}

void PluginEditor::resized() {
    titleLabel.setBounds(16, 8, 300, 36);
    for (int i = 0; i < 6; i++)
        strips[i]->setBounds(10 + i * 130, 60, 124, 396);

    int gx = 806;
    globalLabel.setBounds(gx, 60, 204, 24);
    auto knobRow = [&](juce::Slider& s, juce::Label& l, int y, int size) {
        s.setBounds(gx + (204 - size) / 2, y, size, size);
        l.setBounds(gx, y + size, 204, 16);
    };
    knobRow(driveKnob, driveLabel, 92, 64);
    knobRow(noiseKnob, noiseLabel, 176, 64);
    knobRow(masterKnob, masterLabel, 260, 64);
    knobRow(degradeKnob, degradeLabel, 344, 88);
}

void PluginEditor::timerCallback() {
    for (int i = 0; i < 6; i++)
        strips[i]->setActivity(proc.voiceActivity[i].load());
}
