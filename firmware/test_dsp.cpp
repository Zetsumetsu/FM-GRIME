// test_dsp.cpp — host-side functional test of grime_dsp.h
// g++ -std=c++17 -O2 -o test_dsp test_dsp.cpp && ./test_dsp
#include <cstdio>
#include "grime_dsp.h"

float grime::sinLUT[grime::LUT_N + 1];

int main() {
  using namespace grime;
  initLUT();

  bool ok = true;
  const float rates[3] = {44100.f, 48000.f, 96000.f};
  const char *names[6] = {"kick", "snare", "hatcl", "hatop", "perc", "wild"};

  for (int ri = 0; ri < 3; ri++) {
    setSampleRate(rates[ri]);
    printf("--- sample rate %.0f Hz ---\n", rates[ri]);

    VoiceState vs[6];
    VoiceParams vp[6];
    GlobalParams gp;
    vp[0].hasPitchCV = true; vp[4].hasPitchCV = true; vp[5].hasPitchCV = true;
    vp[2].hasDecayCV = true; vp[3].hasDecayCV = true; vp[1].hasCharCV = true;
    uint32_t seed = 0x12345678;
    float peak[6] = {0};
    int seconds = (int)SR();

    for (int vi = 0; vi < 6; vi++) {
      VoiceType t = (VoiceType)vi;
      for (int k = 0; k < 3; k++) {
        vp[vi].pitchKnob = k * 0.5f;
        vp[vi].decayKnob = 0.2f + 0.3f * k;
        vp[vi].charKnob = k * 0.5f;
        vp[vi].pitchCV = 0.5f; vp[vi].decayCV = 0.5f; vp[vi].charCV = 0.5f;
        gp.noise = 0.2f + 0.3f * k;
        gp.drive = 0.3f * k;
        computeCoefs(t, vs[vi], vp[vi]);
        triggerVoice(t, vs[vi], vp[vi], gp, seed);
        for (int n = 0; n < seconds; n++) {
          float s = renderVoice(t, vs[vi], vp[vi], gp);
          if (!__builtin_isfinite(s)) {
            printf("NON-FINITE %s k=%d n=%d\n", names[vi], k, n);
            ok = false; break;
          }
          float a = fabsf(s);
          if (a > peak[vi]) peak[vi] = a;
        }
        if (!ok) break;
      }
      if (!ok) break;
      // retrigger-while-sounding stress
      triggerVoice(t, vs[vi], vp[vi], gp, seed);
      for (int n = 0; n < 500; n++) {
        float s = renderVoice(t, vs[vi], vp[vi], gp);
        if (!__builtin_isfinite(s)) { printf("RETRIG NON-FINITE %s\n", names[vi]); ok = false; break; }
        if (n == 250) triggerVoice(t, vs[vi], vp[vi], gp, seed);
      }
    }

    for (int vi = 0; vi < 6; vi++) {
      printf("voice %-6s peak %.3f %s\n", names[vi], peak[vi],
             (peak[vi] > 0.05f && peak[vi] <= 1.0f) ? "OK" : "SUSPECT");
      if (!(peak[vi] > 0.05f && peak[vi] <= 1.0f)) ok = false;
    }
  }

  // choke: closed hat must silence open hat's envelope
  {
    setSampleRate(44100.f);
    VoiceState h[2]; VoiceParams hp; GlobalParams gg; uint32_t cseed = 1;
    computeCoefs(HATOP, h[1], hp);
    triggerVoice(HATOP, h[1], hp, gg, cseed);
    for (int n = 0; n < 1000; n++) renderVoice(HATOP, h[1], hp, gg);
    float before = h[1].envAmp;
    h[1].envAmp = 0.0f;  // what engine.trigger does on choke
    float after = renderVoice(HATOP, h[1], hp, gg);
    printf("choke: env %.3f -> out %.4f %s\n", before, after,
           (before > 0.01f && fabsf(after) < 0.001f) ? "OK" : "FAIL");
    if (!(before > 0.01f && fabsf(after) < 0.001f)) ok = false;
  }

  // degrade bus sweep: knob 0→1 with CV, sine input
  {
    DegradeState ds;
    for (int d = 0; d <= 20; d++) {
      float knob = d / 20.0f;
      for (int n = 0; n < 2000; n++) {
        float y = degradeSample(sinf(n * 0.05f) * 0.5f, knob, 0.5f, ds);
        if (!__builtin_isfinite(y)) { printf("DEGRADE NON-FINITE d=%d\n", d); ok = false; break; }
      }
    }
    DegradeState ds2;
    float y = 0;
    for (int n = 0; n < 100; n++) y = degradeSample(0.3f, 1.0f, 0.0f, ds2);
    printf("degrade full: 0.3 -> %.3f (expect 0.250: 4-bit end of remapped range)\n", y);
    if (fabsf(y - 0.25f) > 0.001f) ok = false;
  }

  printf(ok ? "DSP TEST PASS\n" : "DSP TEST FAIL\n");
  return ok ? 0 : 1;
}
