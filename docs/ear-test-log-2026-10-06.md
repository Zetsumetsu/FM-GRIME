# FM GRIME — Ableton ear-test log (2026-10-06)

Draft prepared for tonight's VST3 session. Fill in the tables as you play —
anything you write under "DSP feedback" feeds straight back into the tuning
constants in `firmware/grime_dsp.h` (and the mirrored copies in the VST3 and
Rack plugin). Perc and wild are the melodic ones — play them from the keyboard.

## Trigger map

| Voice | MIDI note | Notes |
|-------|-----------|-------|
| V1 Kick | C1 (36) | root trigger |
| V2 Snare | D1 (38) or E1 (40) | |
| V3 Hat closed | F#1 (42) | choked with hat open |
| V4 Hat open | A#1 (46) | choked with hat closed |
| V5 Perc | A1 (45) | transposes melodically — play riffs |
| V6 Wild | D#1 (39) | transposes melodically — play riffs |

Velocity = hit level. Trigger note transposes the voice relative to its root.

## Outputs

- 6 mono buses = voices dry (pre-degrade) — route to separate Ableton tracks
- Stereo main = full mix post-degrade

## 28 automatable params

Per voice (18): pitch / decay / character × 6 voices.
Globals (4): FM Drive, Noise, Master, Degrade.
Mutes (6): one per voice.

## Kick quick fix (from tonight)

Hearing a tick on C1 = the attack click with no body behind it. Tick → thump:
decay up (~halfway), character up (~two-thirds), pitch down.

## Voice capture log

| Voice | Sweet-spot knobs (pitch / decay / char) | Sounds good? | DSP feedback (ranges, curves, click level) |
|-------|------------------------------------------|--------------|---------------------------------------------|
| Kick | / / | | |
| Snare | / / | | |
| Hat cl | / / | | |
| Hat op | / / | | |
| Perc | / / | | |
| Wild | / / | | |

Things to listen for per voice: pitch knob range wide enough? decay minimum
still musical? character knob travel — dead zones at either end? (Character is
engine-specific: kick = index depth + wavefold; snare = snap→snarl; hats =
tight↔washy; perc = woody→glassy; wild = wobble→ratio chaos.)

## DEGRADE audition

- First half: bit ladder 16→12→8→6→4→2→1, stepped. Do the steps read as a
  chiptune ladder, or do any steps jump too hard?
- Past noon: sample-rate crush fades in smoothly underneath. Is the noon
  handoff smooth?
- Full CW: full destruction arc — still musical, or just mush?

## The three open design decisions — what to listen for tonight

1. **FM DRIVE (offset vs. scaler).** As you dial each voice's character,
   notice the moment you want the whole kit filthier: do you want DRIVE to
   *add dirt on top of* your per-voice settings (offset — keeps their relative
   balance), or to *rescale the whole kit* (scaler — flattens differences)?
   Recommendation on record: offset.
2. **Accents.** Velocity already sets hit level. While playing patterns, does
   the kit feel expressively thin without a dedicated accent behavior — or is
   velocity enough? Lean on record: skip accents, firmware stays lean.
3. **Spare 8th DAC channel.** If you catch yourself wanting a click/cue out
   for live timing while playing, that's the spare channel's job — note it.

## After the session

Hand the "DSP feedback" column back — each note becomes a tuning-constant
change in `grime_dsp.h` (single source of truth; VST3 and Rack copies sync
from it).

## 2026-10-08 — feedback applied (tuning round 1)

- **Kick:** pitch range 40–150 Hz → **32–150 Hz** (low end now reaches ~B1 thump).
- **Snare:** pitch range 150–380 Hz → **62–380 Hz** (starts ~B1 like the kick; top unchanged).
- **Degrade:** full knob sweep remapped to the old 0–0.35 zone — the whole
  travel is now the musical bit ladder (16→12→8→6→4 bits), no more dead/mush
  zone past 0.36. Applies to knob + CV together (firmware, VST3, Rack).
- **FM Drive:** confirmed additive offset (adds dirt on top, keeps relative
  balance) — already the implementation, no change.
- **Accents:** skipped, velocity covers it — no change.
- **Spare DAC:** left unassigned — no change.
- **GUI:** custom editor built — dark instrument panel, 6 voice strips with
  knobs + glowing mute/activity buttons, global strip with big degrade knob.
  All 28 params still automatable.

## 2026-10-08 — GUI crash fix

Ableton crashed on editor open (then on plugin load/scan): `setSize()` was
called at the top of the editor constructor, which fires `resized()`
synchronously while the voice strips were still null -> segfault. Fixed by
creating all components first and calling `setSize()` last. Also initialized
the voice-activity atoms to 0. Verified with a headless editor open + full
offscreen paint test (AddressSanitizer clean).
