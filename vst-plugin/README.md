# FM GRIME — VST3 plugin (DAW version)

The FM GRIME percussion synth as a DAW instrument (Ableton Live etc.).
Same DSP core as the Teensy firmware and the VCV Rack plugin
(`src/grime_dsp.h` is a snapshot of `../firmware/grime_dsp.h` — keep in sync),
re-controlled for the DAW.

## Playing it

- **MIDI notes trigger the voices** (GM-style drum map):
  - C1 (36) — Kick · D1 (38) / E1 (40) — Snare · F#1 (42) — Hat closed
  - A#1 (46) — Hat open · A1 (45) — Perc · D#1 (39) — Wild
- The trigger note **transposes** the voice relative to its root note, so the
  perc and wild voices play melodically from the keyboard.
- **Velocity** sets the hit level.
- All 28 parameters are automatable: 18 voice knobs (pitch/decay/character),
  FM Drive, Noise, Master, Degrade, and 6 mutes.
- **Outputs** (multi-out): stereo Main (post-degrade) + 6 mono voice buses
  (pre-degrade). In Ableton, route them to separate tracks.
- v0.1 uses the host's generic parameter UI — a custom GUI is a later step.

## Build

Requires CMake 3.22+, a C++ compiler, and (first time only) ~100MB for JUCE.

```sh
# macOS (Xcode) / Windows (VS2022) / Linux — builds VST3.
# Add -DFORMATS="VST3 AU" on macOS for Audio Units.
cmake -B build
cmake --build build --config Release
```

The build fetches JUCE 8.0.6 automatically (or point `JUCE_DIR` at a local
checkout and pass `-DFMGRIME_FETCH_JUCE=OFF`).

Install the resulting `.vst3` bundle into your plugin folder and rescan
in your DAW.
