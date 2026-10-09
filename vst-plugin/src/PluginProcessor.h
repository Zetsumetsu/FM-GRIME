#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "grime_dsp.h"

// FM GRIME for DAWs (VST3) — same DSP core as the Teensy firmware and the
// VCV Rack plugin, re-controlled for the DAW:
//   - MIDI note triggers (GM-style drum map); note number transposes the
//     voice relative to its root note; velocity sets the hit level.
//   - 28 automatable parameters (18 voice knobs + 4 global + 6 mutes).
//   - Multi-out: stereo main (post-degrade) + 6 mono voice buses.
// v0.1 has no custom editor — the host's generic parameter UI is used.

class FmGrimeProcessor : public juce::AudioProcessor {
public:
    FmGrimeProcessor();
    ~FmGrimeProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "FM GRIME"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // per-voice trigger activity (1 on trigger, decays) — drives the editor LEDs
    std::atomic<float> voiceActivity[6];
    // kick HF energy (0..1) — GUI-only meter for the logo flash; audio untouched
    std::atomic<float> kickHfActivity{0.0f};

private:
    // MIDI drum map: note -> voice (order: kick, snare, hatCL, hatOP, perc, wild)
    static int noteToVoice(int midiNote);
    static int voiceRootNote(int voice);  // transpose reference for noteToVoice map
    void triggerVoice(int voice, int midiNote, float velocity);

    grime::VoiceState vs[6];
    grime::VoiceParams vp[6];
    grime::GlobalParams gp;
    grime::DegradeState dstate;
    uint32_t seed = 0xC10C1E;
    float velLevel[6] = {1.f, 1.f, 1.f, 1.f, 1.f, 1.f};
    float lastDecayK[6];
    bool coefsDirty = true;
    double lastSampleRate = 44100.0;
    float kickHfEnv = 0.f;   // HF envelope state (logo meter only)
    float kickHpPrev = 0.f;  // differentiator state (logo meter only)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FmGrimeProcessor)
};
