# FM GRIME firmware v0.1

Teensy 4.1 · 6 fixed-engine FM percussion voices · CS42448 over TDM.

## Files

- `fm_grime.ino` — sketch: audio engine (custom `AudioStream`), control scanning, setup/loop
- `grime_dsp.h` — pure C++ DSP core (no Arduino deps; host-testable)
- `grime_config.h` — pin map, mux channel map, defaults
- `test_dsp.cpp` — host-side functional test (see below)

## Routing

| TDM ch | Signal |
|--------|--------|
| 0–5 | individual voice outs, pre-degrade (kick, snare, hat cl, hat op, perc, wild) |
| 6 | main mix, post-degrade |
| 7 | spare (silent) |

## Build

1. Install Teensyduino (Arduino IDE).
2. Open `fm_grime.ino`.
3. Board: **Teensy 4.1**, USB Type: **Audio**, CPU Speed: **600 MHz**, Optimize: **Faster**.
4. Compile & upload.

## Hardware assumptions (see `grime_config.h`)

- Trigger inputs conditioned to 3.3V (divider + clamp); rising-edge detected.
- All CV inputs scaled to 0–3.3V at the ADC.
  - Pitch CV: 0–5V → 1V/oct (kick, perc, wild)
  - Decay CV: 0–5V extends decay (hats)
  - Char CV: 0–5V adds to character (snare)
  - Degrade CV: fixed depth = half knob range
- Mute buttons to GND (`INPUT_PULLUP`); illuminated buttons on PWM pins.
- 2× CD74HC4067 with shared select lines (pins 30–33), SIG on A0/A1.
- **Do not use pins 7, 8, 20, 21, 23 for GPIO** (TDM audio).

## DSP test (no hardware needed)

```sh
g++ -std=c++17 -O2 -o test_dsp test_dsp.cpp && ./test_dsp
```

Renders 1s per voice across knob/CV sweeps, stress-tests retriggering,
verifies the hat choke, and sweeps the degrade bus. Checks for non-finite
samples and sane peak levels.

## Status

v0.1 — first run. DSP core passes host tests. **Not yet run on hardware.**
Tuning parameters (pitch ranges, decay ranges, character curves, voice gains)
are first guesses in `grime_dsp.h` — expect to tune by ear once it makes noise.
