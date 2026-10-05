# FM GRIME — design record

Locked 2026-10-05 unless marked otherwise. Design leadership: assistant proposes, he reacts.

## Concept

Teensy 4.1-based FM percussion synth for Eurorack. Six slim FM voices, big performable degrade/bitcrush control with CV, trigger inputs, hands-on glitch-friendly character in the spirit of the PBD (Progressive Bitcrush Delay). Fully FM: every voice is FM synthesis — no samples, no white-noise generator. Noise-like textures come from FM feedback and inharmonic stacks.

## Voice architecture

Six voices, **fixed engines** per voice (keeps it immediate, no menu diving).

| Voice | Engine |
|-------|--------|
| V1 KICK | 2-op FM. Pitch-enveloped carrier; mod-index envelope runs hot at the attack for a natural click. Character = index depth + post wavefold: clean FM thump → filthy snarling FM. |
| V2 SNARE | FM body + FM-feedback noise burst (modulator with feedback = genuine noise spectra from pure FM). Character = tight snap → crushed snarl. |
| V3 HAT CL | Inharmonic-ratio FM — metallic, glassy. Choked with HAT OP. Character = tight metallic ↔ washy. |
| V4 HAT OP | Same engine as closed, longer decay. Choked with HAT CL. |
| V5 PERC | 2-op FM blip, wide pitch range. Character = FM index: woody low half (toms, conga-ish) → glassy digital top half (claves, zaps). |
| V6 WILD | FM sweep engine: every trigger fires a pitch envelope plus a modulator-ratio sweep. Character = sweep madness: gentle FM wobble → full ratio chaos with slight per-hit random detune. Laser zaps, droid blips, metallic chirps. |

**Per voice:** trigger in · pitch knob · decay knob · character knob (engine-specific, see above) · illuminated mute button (doubles as the activity LED) · individual out (pre-degrade).

**Mute/activity LED behavior (2026-10-05):** playing = dark at rest, bright flash on trigger decaying with the envelope; muted = dim steady red glow (reads at a glance in the dark), still flashes brighter on incoming triggers so you can see a muted voice receiving hits.

**CV inputs (2026-10-05):** PITCH CV on kick, perc, wild — 1V/oct, pitch knob = base offset. DECAY CV on closed/open hats — 0–5V extends decay for dynamic hat patterns. CHAR CV on snare — sweeps snap→snarl so ghost notes stay clean while accents crush.

Hats stay crisp by design — clean hats make the nasty kick/snare hit harder.

## Global section

- **FM DRIVE** — mod-index control across the FM voices. Behavior TBD: offset added on top of per-voice character (recommended — dial in each voice, then ride one knob to dirty the kit) vs. master scaler.
- **NOISE** — FM-feedback noisiness (no white-noise source; the label describes what you hear).
- **MASTER** — main level.
- **DEGRADE** — one big knob + CV in (fixed depth, no attenuator — attenuate externally). Tuned curve: first half walks bit depth down in stepped stages (16→12→8→6→4→2→1, chiptune ladder); past noon, sample-rate crush fades in smoothly underneath. Post-mix, so it glues the kit. Bits + rate linked on the single knob by design — one gesture, full destruction arc, useful under fire.
- **MAIN OUT** — crushed stereo/mono mix (post-degrade).

## I/O summary

- 6× trigger in (conditioned: Teensy pins are 3.3V, not 5V-tolerant — divider/transistor per input, standard practice)
- 6× individual voice out (pre-degrade) + 1× main out (post-degrade) = 7 DAC channels used
- 1× degrade CV in (conditioned to 3.3V ADC range)
- 3× pitch CV in — kick, perc, wild (1V/oct, conditioned; knob = base offset)
- 2× decay CV in — closed/open hats (0–5V extends decay)
- 1× char CV in — snare (sweeps snap→snarl)
- 22 pots + 7 CV = 29 analog inputs (of 32 mux channels)

## Hardware

- **MCU:** Teensy 4.1 (i.MX RT1062 @ 600 MHz). DSP load estimate: 5–15% at 44.1 kHz — six 2-op voices, envelopes, wavefolding, and the degrade bus are trivial for the M7.
- **DAC:** CS42448 codec over TDM — 8 DAC outputs (7 used, 1 spare; spare could become a click/cue out later). Supported directly by the Teensy Audio Library (`AudioOutputTDM` + `AudioControlCS42448`). Quality-over-price pick. QFP-48, 0.5 mm pitch — drag-solderable or fab-assembled; needs clean analog layout per datasheet.
- **Pots/CV:** 2× CD74HC4067 16-channel analog muxes (29 channels used of 32: 22 pots + 7 CV) — standard DIY approach; Teensy 4.1's 18 ADC pins aren't enough alone.
- **Buttons/LEDs:** 6 illuminated mute buttons (integrated red LEDs) on GPIO — plenty of pins.
- **Power:** Eurorack ±12 V regulated down (5 V for Teensy/DAC); modest current draw.

## Panel

32HP, 3U, minimal Grayscale-style (matte light gray, thin black labels). Six voice strips left to right, global section on the right with the large DEGRADE knob. Title: FM GRIME. Render: `panel/fm-grime-panel-v4.jpg` (pixel-precise draft; final fab file may tweak).

## Name

**FM GRIME** — settled 2026-10-05, cleared (no existing module by that name; BITROT and ALIAS were rejected for conflicts). Was "Glitch Kit" during early design.

## Open items

- FM DRIVE: offset vs. master scaler (offset recommended)
- Accent handling: undecided whether it gets a look-in at all
- Spare 8th DAC channel: unassigned
