#include "PluginProcessor.h"
#include "PluginEditor.h"

// sine LUT storage (declared extern in grime_dsp.h)
float grime::sinLUT[grime::LUT_N + 1];

// MIDI drum map. Root note plays the voice at its Pitch knob value;
// other mapped notes transpose relative to the root.
int FmGrimeProcessor::noteToVoice(int midiNote) {
    switch (midiNote) {
        case 36:                                 return 0;  // C1 kick
        case 38: case 40:                        return 1;  // D1/E1 snare
        case 42:                                 return 2;  // F#1 hat closed
        case 46:                                 return 3;  // A#1 hat open
        case 45:                                 return 4;  // A1 perc
        case 39:                                 return 5;  // D#1 wild
        default:                                 return -1;
    }
}

int FmGrimeProcessor::voiceRootNote(int voice) {
    static const int roots[6] = {36, 38, 42, 46, 45, 39};
    return roots[voice];
}

juce::AudioProcessorValueTreeState::ParameterLayout FmGrimeProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    const char* vn[6] = {"Kick", "Snare", "Hat closed", "Hat open", "Perc", "Wild"};
    const char* vid[6] = {"kick", "snare", "hatcl", "hatop", "perc", "wild"};
    const char* kn[3] = {"Pitch", "Decay", "Character"};
    const char* kid[3] = {"pitch", "decay", "char"};
    for (int i = 0; i < 6; i++) {
        for (int k = 0; k < 3; k++) {
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                juce::String(vid[i]) + "_" + kid[k],
                std::string(vn[i]) + " " + kn[k],
                juce::NormalisableRange<float>(0.f, 1.f), 0.5f));
        }
    }
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "drive", "FM Drive", juce::NormalisableRange<float>(0.f, 1.f), 0.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "noise", "Noise", juce::NormalisableRange<float>(0.f, 1.f), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "master", "Master", juce::NormalisableRange<float>(0.f, 1.f), 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "degrade", "Degrade", juce::NormalisableRange<float>(0.f, 1.f), 0.f));
    for (int i = 0; i < 6; i++) {
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::String(vid[i]) + "_mute", std::string(vn[i]) + " mute", false));
    }
    return {params.begin(), params.end()};
}

FmGrimeProcessor::FmGrimeProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Main", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Kick", juce::AudioChannelSet::mono(), true)
                         .withOutput("Snare", juce::AudioChannelSet::mono(), true)
                         .withOutput("Hat Cl", juce::AudioChannelSet::mono(), true)
                         .withOutput("Hat Op", juce::AudioChannelSet::mono(), true)
                         .withOutput("Perc", juce::AudioChannelSet::mono(), true)
                         .withOutput("Wild", juce::AudioChannelSet::mono(), true)),
      apvts(*this, nullptr, "FMGRIME", createParameterLayout()) {
    grime::initLUT();
    for (int i = 0; i < 6; i++) { lastDecayK[i] = -1.f; voiceActivity[i].store(0.f); }
}

void FmGrimeProcessor::prepareToPlay(double sampleRate, int) {
    grime::setSampleRate((float) sampleRate);
    lastSampleRate = sampleRate;
    coefsDirty = true;
}

juce::AudioProcessorEditor* FmGrimeProcessor::createEditor() {
    return new PluginEditor(*this);
}

bool FmGrimeProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    // v0.1 requires the full native layout: stereo main + 6 mono voice buses
    if (layouts.outputBuses.size() != 7) return false;
    if (layouts.outputBuses[0] != juce::AudioChannelSet::stereo()) return false;
    for (int i = 1; i < 7; i++)
        if (layouts.outputBuses[i] != juce::AudioChannelSet::mono()) return false;
    return true;
}

void FmGrimeProcessor::triggerVoice(int v, int midiNote, float velocity) {
    if (vp[v].muted) return;
    vp[v].transposeMult = std::pow(2.f, (midiNote - voiceRootNote(v)) / 12.f);
    velLevel[v] = 0.25f + 0.75f * velocity;
    voiceActivity[v].store(1.0f);
    grime::triggerVoice((grime::VoiceType) v, vs[v], vp[v], gp, seed);
    if (v == grime::HATCL) vs[grime::HATOP].envAmp = 0.f;  // choke pair
    if (v == grime::HATOP) vs[grime::HATCL].envAmp = 0.f;
}

void FmGrimeProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    const int numSamples = buffer.getNumSamples();
    const char* vid[6] = {"kick", "snare", "hatcl", "hatop", "perc", "wild"};
    const char* kid[3] = {"pitch", "decay", "char"};

    for (int i = 0; i < 6; i++) {
        for (int k = 0; k < 3; k++) {
            float v = apvts.getRawParameterValue(juce::String(vid[i]) + "_" + kid[k])->load();
            if (k == 0) vp[i].pitchKnob = v;
            else if (k == 1) vp[i].decayKnob = v;
            else vp[i].charKnob = v;
        }
        vp[i].muted = apvts.getRawParameterValue(juce::String(vid[i]) + "_mute")->load() > 0.5f;
    }
    gp.drive = apvts.getRawParameterValue("drive")->load();
    gp.noise = apvts.getRawParameterValue("noise")->load();
    gp.master = apvts.getRawParameterValue("master")->load();
    gp.degradeKnob = apvts.getRawParameterValue("degrade")->load();
    gp.degradeCV = 0.f;  // no CV input in the DAW; automate the Degrade parameter

    for (const auto meta : midi) {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn()) {
            int v = noteToVoice(msg.getNoteNumber());
            if (v >= 0) triggerVoice(v, msg.getNoteNumber(), msg.getFloatVelocity());
        }
    }

    bool dirty = coefsDirty;
    for (int i = 0; i < 6; i++)
        if (std::abs(vp[i].decayKnob - lastDecayK[i]) > 1e-4f) dirty = true;
    if (dirty) {
        for (int i = 0; i < 6; i++) {
            grime::computeCoefs((grime::VoiceType) i, vs[i], vp[i]);
            lastDecayK[i] = vp[i].decayKnob;
        }
        coefsDirty = false;
    }

    auto mainBus = getBusBuffer(buffer, false, 0);
    float* mainL = mainBus.getWritePointer(0);
    float* mainR = mainBus.getNumChannels() > 1 ? mainBus.getWritePointer(1) : mainL;
    float* voiceOut[6];
    for (int i = 0; i < 6; i++)
        voiceOut[i] = getBusBuffer(buffer, false, i + 1).getWritePointer(0);

    // kick HF meter decay (~80 ms fall), applied per-sample in the loop below
    const float hfDec = std::pow(0.5f, 1.0f / (float) (lastSampleRate * 0.08));

    for (int n = 0; n < numSamples; ++n) {
        float mix = 0.f;
        for (int i = 0; i < 6; i++) {
            float s = vp[i].muted ? 0.f
                      : grime::renderVoice((grime::VoiceType) i, vs[i], vp[i], gp) * velLevel[i];
            if (i == 0) {
                // logo meter: HF energy of the kick (differentiator ~= highpass
                // + peak envelope). Read-only tap — the audio path is untouched.
                float hp = s - kickHpPrev;
                kickHpPrev = s;
                float a = std::abs(hp);
                kickHfEnv = a > kickHfEnv ? a : kickHfEnv * hfDec;
            }
            voiceOut[i][n] = s;
            mix += s;
        }
        float m = grime::degradeSample(mix * 0.4f, gp.degradeKnob, gp.degradeCV, dstate)
                  * gp.master;
        m = grime::clampf(m, -1.f, 1.f);
        mainL[n] = m;
        mainR[n] = m;
    }

    // decay the activity LEDs (~120 ms fall)
    float actMul = std::pow(0.5f, (float) numSamples / (float) (lastSampleRate * 0.12));
    for (int i = 0; i < 6; i++)
        voiceActivity[i].store(voiceActivity[i].load() * actMul);

    // publish the kick HF meter for the logo flash (gain tuned so a hard
    // kick transient reads near 1; purely a GUI value)
    kickHfActivity.store(juce::jmin(kickHfEnv * 6.0f, 1.0f));
}

void FmGrimeProcessor::getStateInformation(juce::MemoryBlock& destData) {
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void FmGrimeProcessor::setStateInformation(const void* data, int sizeInBytes) {
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

// Entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new FmGrimeProcessor();
}
