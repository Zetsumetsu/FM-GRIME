// grime_dsp.h — FM GRIME pure DSP core.
// No Arduino / Audio dependencies: compile-checkable with plain g++.
// Voices are fixed 2-op FM engines per the design record (2026-10-05).
#pragma once

#include <cstdint>
#include <cmath>

namespace grime {

inline float clampf(float x, float a, float b) { return x < a ? a : (x > b ? b : x); }

static constexpr float FS = 44100.0f;

// ---------------------------------------------------------------- sine LUT
static constexpr int LUT_N = 2048;
extern float sinLUT[LUT_N + 1];  // defined once by the host (.ino or test)

inline void initLUT() {
  for (int i = 0; i <= LUT_N; i++)
    sinLUT[i] = sinf(2.0f * 3.14159265358979f * (float)i / (float)LUT_N);
}

// phase in cycles [0,1)
inline float fastSin(float cycles) {
  float p = cycles - floorf(cycles);
  float idx = p * (float)LUT_N;
  int i = (int)idx;
  float f = idx - (float)i;
  return sinLUT[i] + f * (sinLUT[i + 1] - sinLUT[i]);
}

// ---------------------------------------------------------------- helpers
// fold x into [-1, 1]
inline float wavefold(float x) {
  x = fmodf(x + 1.0f, 4.0f);
  if (x < 0.0f) x += 4.0f;
  x -= 1.0f;  // now in [-1, 3)
  return (x <= 1.0f) ? x : (2.0f - x);
}

// exponential decay coefficient: envelope reaches -60dB after `seconds`
inline float decayCoef(float seconds) {
  if (seconds < 0.0005f) seconds = 0.0005f;
  return powf(0.001f, 1.0f / (seconds * FS));
}

// exponential knob mapping
inline float expMap(float k, float lo, float hi) {
  return lo * powf(hi / lo, clampf(k, 0.0f, 1.0f));
}

// deterministic PRNG (wild voice per-hit detune)
inline float frand(uint32_t &seed) {
  seed = seed * 1664525u + 1013904223u;
  return (float)(seed >> 8) * (1.0f / 16777216.0f);
}

// ---------------------------------------------------------------- params
enum VoiceType { KICK = 0, SNARE = 1, HATCL = 2, HATOP = 3, PERC = 4, WILD = 5 };

struct VoiceParams {
  float pitchKnob = 0.5f, decayKnob = 0.5f, charKnob = 0.5f;
  float pitchCV = 0.0f;   // 0..1  (0–5V → 1V/oct)
  float decayCV = 0.0f;   // 0..1  (0–5V extends decay)
  float charCV = 0.0f;    // 0..1  (0–5V adds to character)
  bool hasPitchCV = false, hasDecayCV = false, hasCharCV = false;
  bool muted = false;
};

struct GlobalParams {
  float drive = 0.0f;       // additive character offset across voices
  float noise = 0.35f;      // global FM-feedback noisiness
  float master = 0.8f;      // main out level
  float degradeKnob = 0.0f; // 0..1
  float degradeCV = 0.0f;   // 0..1, fixed depth
};

struct VoiceState {
  float phaseC = 0, phaseM = 0, phaseN = 0;
  float envAmp = 0, envAux = 0, envB = 0;
  float coefAmp = 1, coefAux = 1, coefB = 1;
  float noisePrev = 0;
  float detune = 1.0f;
};

// decay time in seconds for the amplitude envelope, per voice
inline float voiceDecaySec(VoiceType t, const VoiceParams &p) {
  switch (t) {
    case KICK:  return expMap(p.decayKnob, 0.08f, 1.20f);
    case SNARE: return expMap(p.decayKnob, 0.08f, 0.80f);
    case HATCL: return expMap(p.decayKnob, 0.03f, 0.35f) * (p.hasDecayCV ? (1.0f + 3.0f * p.decayCV) : 1.0f);
    case HATOP: return expMap(p.decayKnob, 0.10f, 1.20f) * (p.hasDecayCV ? (1.0f + 3.0f * p.decayCV) : 1.0f);
    case PERC:  return expMap(p.decayKnob, 0.05f, 1.00f);
    case WILD:  return expMap(p.decayKnob, 0.10f, 1.50f);
  }
  return 0.3f;
}

// recompute envelope coefficients (call at control rate)
inline void computeCoefs(VoiceType t, VoiceState &s, const VoiceParams &p) {
  float d = voiceDecaySec(t, p);
  s.coefAmp = decayCoef(d);
  switch (t) {
    case KICK:  s.coefAux = decayCoef(0.008f); s.coefB = decayCoef(0.018f); break; // pitch / index envs
    case SNARE: s.coefAux = decayCoef(0.025f); s.coefB = 1.0f; break;              // burst env
    case WILD:  s.coefAux = decayCoef(d * 1.2f); s.coefB = 1.0f; break;           // sweep env
    default:    s.coefAux = 1.0f; s.coefB = 1.0f; break;
  }
}

// base frequency from pitch knob (+ 1V/oct CV where fitted)
inline float voiceBaseFreq(VoiceType t, const VoiceParams &p) {
  float f;
  switch (t) {
    case KICK:  f = expMap(p.pitchKnob, 40.0f, 150.0f); break;
    case SNARE: f = expMap(p.pitchKnob, 150.0f, 380.0f); break;
    case HATCL:
    case HATOP: f = expMap(p.pitchKnob, 500.0f, 1400.0f); break;
    case PERC:  f = expMap(p.pitchKnob, 80.0f, 1200.0f); break;
    case WILD:  f = expMap(p.pitchKnob, 60.0f, 700.0f); break;
    default:    f = 220.0f;
  }
  if (p.hasPitchCV) {
    float semis = p.pitchCV * 5.0f * 12.0f;  // 0–5V → 0–60 semitones
    f *= powf(2.0f, semis / 12.0f);
  }
  return clampf(f, 15.0f, 5000.0f);
}

inline float charEff(const VoiceParams &p, const GlobalParams &g) {
  float c = p.charKnob + g.drive;
  if (p.hasCharCV) c += p.charCV * 0.8f;
  return clampf(c, 0.0f, 1.0f);
}

// ---------------------------------------------------------------- trigger
inline void triggerVoice(VoiceType t, VoiceState &s, const VoiceParams &p,
                         const GlobalParams &g, uint32_t &seed) {
  (void)p;
  s.envAmp = 1.0f; s.envAux = 1.0f; s.envB = 1.0f;
  s.phaseC = 0.0f; s.phaseM = 0.0f; s.phaseN = 0.0f;
  s.noisePrev = 0.0f;
  if (t == WILD) {
    float ce = charEff(p, g);
    s.detune = 1.0f + (frand(seed) - 0.5f) * 0.04f * (0.3f + ce);
  }
}

// ---------------------------------------------------------------- render
// One sample. Returns voice output (~[-1,1]). Caller handles mute.
inline float renderVoice(VoiceType t, VoiceState &s, const VoiceParams &p,
                         const GlobalParams &g) {
  if (s.envAmp < 0.0002f) { s.envAmp = 0.0f; return 0.0f; }
  const float ce = charEff(p, g);
  float y = 0.0f;

  switch (t) {
    case KICK: {
      float base = voiceBaseFreq(t, p);
      float f = base * (1.0f + 2.5f * s.envAux);
      float idx = (1.5f + 9.0f * ce) * s.envB;              // hot at attack = click
      float m = fastSin(s.phaseM + g.noise * 0.25f * s.noisePrev);
      s.phaseM += f / FS; s.noisePrev = m;
      float c = fastSin(s.phaseC + idx * m);
      s.phaseC += f / FS;
      y = wavefold(c * (1.0f + 5.0f * ce)) * 0.9f;
      s.envAux *= s.coefAux; s.envB *= s.coefB;
      break;
    }
    case SNARE: {
      float bodyF = voiceBaseFreq(t, p);
      float mB = fastSin(s.phaseM);
      s.phaseM += bodyF * 1.5f / FS;
      float body = fastSin(s.phaseC + (2.0f + 5.0f * ce) * mB);
      s.phaseC += bodyF / FS;
      // FM-feedback noise burst: genuine noise spectra from pure FM
      float fbAmt = 2.5f + 2.0f * g.noise;
      float n = fastSin(s.phaseN + fbAmt * s.noisePrev);
      s.phaseN += 2800.0f / FS; s.noisePrev = n;
      float burstAmt = (0.25f + 0.9f * ce) * (0.4f + 0.8f * g.noise);
      y = wavefold((body * 0.75f + n * s.envAux * burstAmt) * (1.0f + 2.0f * ce)) * 0.8f;
      s.envAux *= s.coefAux;
      break;
    }
    case HATCL:
    case HATOP: {
      float base = voiceBaseFreq(t, p);
      float m = fastSin(s.phaseM + g.noise * 0.8f * s.noisePrev);
      s.phaseM += base * 2.76f / FS;  // inharmonic ratio → metallic
      s.noisePrev = m;
      float c = fastSin(s.phaseC + (2.5f + 6.0f * ce) * m);
      s.phaseC += base / FS;
      y = c * 0.5f;
      break;
    }
    case PERC: {
      float base = voiceBaseFreq(t, p);
      float m = fastSin(s.phaseM + g.noise * 0.4f * s.noisePrev);
      s.phaseM += base / FS; s.noisePrev = m;
      float c = fastSin(s.phaseC + (0.5f + 8.0f * ce) * m);  // woody → glassy
      s.phaseC += base / FS;
      y = wavefold(c * (1.0f + 1.5f * ce)) * 0.7f;
      break;
    }
    case WILD: {
      float f0 = voiceBaseFreq(t, p);
      float sw = s.envAux;
      float f = f0 * powf(2.0f, (0.5f + 3.5f * ce) * sw) * s.detune;
      float ratio = 1.0f + ce * 7.0f * sw;                    // ratio chaos
      float m = fastSin(s.phaseM + g.noise * 0.5f * s.noisePrev);
      s.phaseM += f * ratio / FS; s.noisePrev = m;
      float c = fastSin(s.phaseC + (1.0f + 7.0f * ce) * (0.3f + 0.7f * sw) * m);
      s.phaseC += f / FS;
      y = wavefold(c * (1.0f + 2.0f * ce)) * 0.7f;
      s.envAux *= s.coefAux;
      break;
    }
  }

  s.envAmp *= s.coefAmp;
  return y * s.envAmp;
}

// ---------------------------------------------------------------- degrade bus
// One big knob: first half = stepped bit ladder (16→1), past noon += smooth
// sample-rate crush. Post-mix. CV adds with fixed depth.
struct DegradeState {
  float held = 0.0f;
  float acc = 0.0f;
};

inline float degradeSample(float x, float knob01, float cv01, DegradeState &st) {
  float d = clampf(knob01 + 0.5f * cv01, 0.0f, 1.0f);  // fixed CV depth
  static const int ladder[7] = {16, 12, 8, 6, 4, 2, 1};
  int bits;
  if (d <= 0.5f) {
    int stage = (int)(d * 2.0f * 6.999f);
    if (stage > 6) stage = 6;
    bits = ladder[stage];
  } else {
    bits = 1;
  }
  float y = x;
  if (bits < 16) {
    float q = (float)(1 << (bits - 1));
    y = roundf(y * q) / q;
  }
  float hold = 1.0f;
  if (d > 0.5f) hold = 1.0f + (d - 0.5f) * 2.0f * 31.0f;  // 1 → 32 samples
  st.acc += 1.0f;
  if (st.acc >= hold) { st.acc = 0.0f; st.held = y; }
  return st.held;
}

}  // namespace grime
