# FM GRIME

FM GRIME — a 6-voice fully-FM Eurorack drum synthesizer on Teensy 4.1: dirty FM kicks, glassy hats, and one big performable degrade knob that bitcrushes the whole kit.

## Status

Design phase. Voice architecture and panel layout are locked; firmware and hardware are next.

## The module

- **32HP, 3U** Eurorack drum synthesizer, Teensy 4.1 based
- **6 fixed-architecture FM voices** — kick, snare, closed hat, open hat, perc, wild — 100% FM, no samples, no noise generator
- **Per voice:** trigger in, pitch / decay / character knobs, mute button, red activity LED, individual out (pre-degrade)
- **Global:** FM drive, noise, master, one big **DEGRADE** knob (stepped bitcrush into sample-rate crush) with CV in, crushed main out
- **Audio:** CS42448 codec over TDM — 8 DAC channels (6 individual + main + 1 spare)
- **Panel:** minimal Grayscale-style, matte light gray

See [docs/DESIGN.md](docs/DESIGN.md) for the full design record and [panel/](panel/) for the panel draft.

## Repo layout

- `README.md` — this file
- `docs/DESIGN.md` — voice architecture, panel, hardware decisions
- `panel/` — panel renders and drawing script
- `.gitignore` — build artifacts, IDE files
