#include "PluginEditor.h"
#include "BinaryData.h"

namespace {
constexpr float kPi = 3.14159265358979323846f;

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

    g.setColour(GuiLab::KNOB);
    g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
    g.setColour(GuiLab::DIM);
    g.drawEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, 1.5f);

    float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    float arcR = radius - 5.0f;
    if (sliderPos > 0.001f) {
        juce::Path arc;
        arc.addArc(centre.x - arcR, centre.y - arcR, arcR * 2.0f, arcR * 2.0f,
                   rotaryStartAngle, angle, true);
        // soft neon glow under the value arc, then the crisp arc on top
        g.setColour(GuiLab::ACCENT.withAlpha(0.22f));
        g.strokePath(arc, juce::PathStrokeType(7.0f));
        g.setColour(GuiLab::ACCENT);
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
    auto b = getLocalBounds().toFloat();
    bool muted = getToggleState();

    if (!GuiLab::USE_FANCY_MUTE) {
        // minimal fallback: flat rect, no rounded corners or alpha blending
        g.setColour(muted ? juce::Colours::darkred : juce::Colours::darkgrey);
        g.fillRect(b);
        g.setColour(juce::Colours::white);
        g.setFont(12.0f);
        g.drawText(muted ? "MUTED" : "MUTE", b, juce::Justification::centred);
        return;
    }

    b = b.reduced(2.0f);
    float act = juce::jmin(1.0f, activity.load());

    g.setColour(muted ? GuiLab::MUTE_BG : juce::Colour(0xff242424));
    g.fillRoundedRectangle(b, 6.0f);

    if (act > 0.01f) {
        // hit flash stays red even though the UI accent is neon green
        g.setColour(GuiLab::HIT.withAlpha(act * (muted ? 0.95f : 0.6f)));
        g.fillRoundedRectangle(b, 6.0f);
    }

    g.setColour(muted ? juce::Colours::white : GuiLab::DIM);
    g.setFont(12.0f);
    g.drawText(muted ? "MUTED" : "MUTE", b, juce::Justification::centred);
}

// ---- VoiceStrip ----

void VoiceStrip::setupKnob(juce::Slider& s, GrimeLookAndFeel& lnf,
                           juce::Label& label, const juce::String& labelText) {
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setRotaryParameters(kPi * 1.25f, kPi * 2.75f, true);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    if (GuiLab::USE_CUSTOM_KNOBS)
        s.setLookAndFeel(&lnf);
    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, GuiLab::DIM);
    label.setFont(11.0f);
}

VoiceStrip::VoiceStrip(FmGrimeProcessor& proc, int voiceIndex, GrimeLookAndFeel& lnf) {
    setupKnob(pitchKnob, lnf, pitchLabel, "PITCH");
    setupKnob(decayKnob, lnf, decayLabel, "DECAY");
    setupKnob(charKnob, lnf, charLabel, "CHAR");

    nameLabel.setText(kVoiceNames[voiceIndex], juce::dontSendNotification);
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setColour(juce::Label::textColourId, GuiLab::TEXT);
    nameLabel.setFont(juce::Font(15.0f, juce::Font::bold));

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
    nameLabel.setBounds(b.removeFromTop(GuiLab::NAME_H));
    b.removeFromTop(GuiLab::NAME_GAP);  // breathing room under the channel name
    auto knobRow = [&](juce::Slider& s, juce::Label& l) {
        auto row = b.removeFromTop(GuiLab::KNOB_ROW_H);
        l.setBounds(row.removeFromBottom(16));
        s.setBounds(row);
    };
    knobRow(pitchKnob, pitchLabel);
    knobRow(decayKnob, decayLabel);
    knobRow(charKnob, charLabel);
    b.removeFromTop(8);
    mute.setBounds(b.removeFromTop(36));
}

// ---- PluginEditor ----

void PluginEditor::setupKnob(juce::Slider& s, GrimeLookAndFeel& lnf,
                             juce::Label& label, const juce::String& labelText) {
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setRotaryParameters(kPi * 1.25f, kPi * 2.75f, true);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    if (GuiLab::USE_CUSTOM_KNOBS)
        s.setLookAndFeel(&lnf);
    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, GuiLab::DIM);
    label.setFont(11.0f);
}

PluginEditor::PluginEditor(FmGrimeProcessor& p)
    : juce::AudioProcessorEditor(p), proc(p) {
    // Embedded artwork (BinaryData). Loaded synchronously here on the message
    // thread — no background threads, no async callbacks (crash-safety).
    bgImage = juce::ImageCache::getFromMemory(BinaryData::slime_bg_jpg,
                                              BinaryData::slime_bg_jpgSize);
    titleImage = juce::ImageCache::getFromMemory(BinaryData::grime_title_jpg,
                                                 BinaryData::grime_title_jpgSize);

    globalLabel.setText("GLOBAL", juce::dontSendNotification);
    globalLabel.setJustificationType(juce::Justification::centred);
    globalLabel.setColour(juce::Label::textColourId, GuiLab::TEXT);
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

    // LAST: setSize() fires resized() synchronously, so every child
    // component above must exist before this line runs.
    setSize(GuiLab::WIN_W, GuiLab::WIN_H);
    if (GuiLab::USE_ACTIVITY_TIMER)
        startTimerHz(30);
}

void PluginEditor::paint(juce::Graphics& g) {
    // slime-wall background, cropped to fill (no stretching)
    if (bgImage.isValid())
        g.drawImage(bgImage, getLocalBounds().toFloat(),
                    juce::RectanglePlacement::fillDestination);
    else
        g.fillAll(juce::Colours::black);
    // dim the artwork so controls stay readable
    g.fillAll(GuiLab::BG_DIM);

    // voice strips: translucent dark forest green + neon hairline on top
    for (int i = 0; i < 6; i++) {
        auto b = strips[i]->getBounds().toFloat();
        g.setColour(GuiLab::STRIP);
        g.fillRoundedRectangle(b, 8.0f);
        g.setColour(GuiLab::ACCENT.withAlpha(0.55f));
        g.fillRect(b.getX() + 10.0f, b.getY(), b.getWidth() - 20.0f, 2.0f);
    }
    auto gb = juce::Rectangle<float>((float) GuiLab::GLOBAL_X, (float) GuiLab::STRIP_Y,
                                     (float) GuiLab::GLOBAL_W, (float) GuiLab::STRIP_H);
    g.setColour(GuiLab::STRIP);
    g.fillRoundedRectangle(gb, 8.0f);
    g.setColour(GuiLab::ACCENT.withAlpha(0.55f));
    g.fillRect(gb.getX() + 10.0f, gb.getY(), gb.getWidth() - 20.0f, 2.0f);

    // title banner, left-justified (flat text fallback if artwork missing)
    if (titleImage.isValid()) {
        g.drawImage(titleImage,
                    juce::Rectangle<float>((float) GuiLab::TITLE_X, (float) GuiLab::TITLE_Y,
                                           (float) GuiLab::TITLE_W, (float) GuiLab::TITLE_H));
    } else {
        g.setColour(GuiLab::ACCENT);
        g.setFont(juce::Font(30.0f, juce::Font::bold));
        g.drawText("FM GRIME", GuiLab::TITLE_X, GuiLab::TITLE_Y,
                   GuiLab::TITLE_W, 40, juce::Justification::centredLeft);
    }
}

void PluginEditor::resized() {
    for (int i = 0; i < 6; i++)
        strips[i]->setBounds(GuiLab::STRIP_X0 + i * GuiLab::STRIP_PITCH,
                             GuiLab::STRIP_Y, GuiLab::STRIP_W, GuiLab::STRIP_H);

    // global panel stacks top-down with a cursor so nothing can overflow
    int gx = GuiLab::GLOBAL_X;
    int y = GuiLab::STRIP_Y;
    globalLabel.setBounds(gx, y, GuiLab::GLOBAL_W, 18);
    y += 22;
    auto smallRow = [&](juce::Slider& s, juce::Label& l) {
        int d = GuiLab::GLOBAL_KNOB_D;
        s.setBounds(gx + (GuiLab::GLOBAL_W - d) / 2, y, d, d);
        y += d + 2;
        l.setBounds(gx, y, GuiLab::GLOBAL_W, 13);
        y += 13 + 4;
    };
    smallRow(driveKnob, driveLabel);
    smallRow(noiseKnob, noiseLabel);
    smallRow(masterKnob, masterLabel);
    int bd = GuiLab::BIG_KNOB_D;
    degradeKnob.setBounds(gx + (GuiLab::GLOBAL_W - bd) / 2, y, bd, bd);
    degradeLabel.setBounds(gx, y + bd + 2, GuiLab::GLOBAL_W, 13);
}

void PluginEditor::timerCallback() {
    for (int i = 0; i < 6; i++)
        strips[i]->setActivity(proc.voiceActivity[i].load());
}
