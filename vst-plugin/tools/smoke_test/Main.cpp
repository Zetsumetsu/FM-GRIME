// Headless smoke test: loads the built FM GRIME.vst3, instantiates it,
// fires MIDI notes, renders audio, and checks the output is sane.
// Not part of the plugin; dev tooling only.
#include <juce_audio_processors/juce_audio_processors.h>
#include <cstdio>
#include <cmath>

int main(int argc, char** argv) {
    if (argc < 2) { std::printf("usage: smoke_test <path-to-.vst3>\n"); return 2; }

    juce::VST3PluginFormat format;
    juce::OwnedArray<juce::PluginDescription> descs;
    format.findAllTypesForFile(descs, argv[1]);
    if (descs.isEmpty()) { std::printf("FAIL: no plugin found in bundle\n"); return 1; }

    juce::String err;
    std::unique_ptr<juce::AudioPluginInstance> inst(
        format.createInstanceFromDescription(*descs[0], 44100.0, 512, err));
    if (!inst) { std::printf("FAIL: instantiate: %s\n", err.toRawUTF8()); return 1; }
    std::printf("instantiated: %s\n", inst->getName().toRawUTF8());
    std::printf("output buses: %d, total out channels: %d\n",
                inst->getBusCount(false), inst->getTotalNumOutputChannels());

    inst->prepareToPlay(44100.0, 512);

    juce::AudioBuffer<float> buf(inst->getTotalNumOutputChannels(), 512);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 36, 0.9f), 0);    // kick
    midi.addEvent(juce::MidiMessage::noteOn(1, 38, 0.8f), 100);  // snare
    midi.addEvent(juce::MidiMessage::noteOn(1, 42, 0.7f), 200);  // hat closed
    midi.addEvent(juce::MidiMessage::noteOn(1, 39, 0.9f), 300);  // wild
    inst->processBlock(buf, midi);

    bool ok = true;
    auto analyze = [&](int ch, const char* name) {
        float peak = 0.f, sum = 0.f;
        bool finite = true;
        auto* d = buf.getReadPointer(ch);
        for (int n = 0; n < buf.getNumSamples(); n++) {
            if (!std::isfinite(d[n])) finite = false;
            peak = std::max(peak, std::abs(d[n]));
            sum += std::abs(d[n]);
        }
        std::printf("%-8s peak=%.3f mean=%.4f finite=%d\n",
                    name, peak, sum / buf.getNumSamples(), (int) finite);
        return finite && peak > 0.01f && peak <= 1.0f;
    };
    ok &= analyze(0, "main L");
    ok &= analyze(1, "main R");
    ok &= analyze(2, "kick");
    ok &= analyze(3, "snare");
    ok &= analyze(4, "hatcl");
    ok &= analyze(7, "wild");

    // let envelopes decay; output must go quiet (no stuck voices)
    juce::MidiBuffer empty;
    for (int b = 0; b < 300; b++) { buf.clear(); inst->processBlock(buf, empty); }
    float tail = 0.f;
    auto* d = buf.getReadPointer(0);
    for (int n = 0; n < buf.getNumSamples(); n++)
        tail = std::max(tail, std::abs(d[n]));
    std::printf("tail after 300 blocks: %.5f %s\n", tail, tail < 0.01f ? "OK" : "SUSPECT");
    ok &= tail < 0.01f;

    std::printf(ok ? "SMOKE TEST PASS\n" : "SMOKE TEST FAIL\n");
    return ok ? 0 : 1;
}
