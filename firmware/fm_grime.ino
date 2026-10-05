// fm_grime.ino — FM GRIME firmware v0.1 (Teensy 4.1)
//
// Six fixed-engine FM percussion voices → CS42448 over TDM.
//   TDM ch 0–5: individual voice outs (pre-degrade)
//   TDM ch 6:   main mix (post-degrade)
//   TDM ch 7:   spare (silent)
//
// Design record: docs/DESIGN.md
// Build: Teensyduino (Arduino IDE), Board "Teensy 4.1", USB Type "Audio",
//        CPU Speed 600 MHz, Optimize "Faster".

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include "grime_config.h"
#include "grime_dsp.h"

float grime::sinLUT[grime::LUT_N + 1];

// ---------------------------------------------------------------- engine
class GrimeEngine : public AudioStream {
public:
  GrimeEngine() : AudioStream(0, NULL) {
    vp[0].hasPitchCV = true;                    // kick
    vp[4].hasPitchCV = true;                    // perc
    vp[5].hasPitchCV = true;                    // wild
    vp[2].hasDecayCV = true;                    // hat closed
    vp[3].hasDecayCV = true;                    // hat open
    vp[1].hasCharCV = true;                     // snare
    gp.noise = DEFAULT_NOISE;
    gp.master = DEFAULT_MASTER;
  }

  grime::VoiceState vs[6];
  grime::VoiceParams vp[6];
  grime::GlobalParams gp;
  grime::DegradeState dstate;
  uint32_t seed = 0xC10C1E;

  void trigger(int i) {
    if (i < 0 || i > 5) return;
    grime::triggerVoice((grime::VoiceType)i, vs[i], vp[i], gp, seed);
    if (i == grime::HATCL) vs[grime::HATOP].envAmp = 0.0f;  // choke pair
    if (i == grime::HATOP) vs[grime::HATCL].envAmp = 0.0f;
  }

  float envLevel(int i) const { return vs[i].envAmp; }

  void update() override {
    audio_block_t *out[7];
    for (int i = 0; i < 7; i++) {
      out[i] = allocate();
      if (!out[i]) {
        for (int j = 0; j < i; j++) release(out[j]);
        return;
      }
    }
    for (int n = 0; n < AUDIO_BLOCK_SAMPLES; n++) {
      float v[6], mix = 0.0f;
      for (int i = 0; i < 6; i++) {
        v[i] = vp[i].muted ? 0.0f
                           : grime::renderVoice((grime::VoiceType)i, vs[i], vp[i], gp);
        mix += v[i];
      }
      float main = grime::degradeSample(mix * 0.4f, gp.degradeKnob, gp.degradeCV, dstate)
                   * gp.master;
      for (int i = 0; i < 6; i++)
        out[i]->data[n] = (int16_t)(grime::clampf(v[i], -1.0f, 1.0f) * 32767.0f);
      out[TDM_CH_MAIN]->data[n] = (int16_t)(grime::clampf(main, -1.0f, 1.0f) * 32767.0f);
    }
    for (int i = 0; i < 7; i++) { transmit(out[i], i); release(out[i]); }
  }
};

GrimeEngine engine;
AudioOutputTDM tdm;
AudioControlCS42448 codec;
AudioConnection ac0(engine, 0, tdm, 0);
AudioConnection ac1(engine, 1, tdm, 1);
AudioConnection ac2(engine, 2, tdm, 2);
AudioConnection ac3(engine, 3, tdm, 3);
AudioConnection ac4(engine, 4, tdm, 4);
AudioConnection ac5(engine, 5, tdm, 5);
AudioConnection ac6(engine, TDM_CH_MAIN, tdm, TDM_CH_MAIN);

// ---------------------------------------------------------------- control
float potSmoothA[16] = {0};
float potSmoothB[16] = {0};
uint8_t trigPrev[6] = {0};
uint32_t trigLock[6] = {0};
uint8_t btnPrev[6] = {1, 1, 1, 1, 1, 1};
uint32_t btnDeb[6] = {0};
float trigFlash[6] = {0};

inline void selectMux(uint8_t ch) {
  digitalWrite(PIN_MUX_S0, (ch >> 0) & 1);
  digitalWrite(PIN_MUX_S1, (ch >> 1) & 1);
  digitalWrite(PIN_MUX_S2, (ch >> 2) & 1);
  digitalWrite(PIN_MUX_S3, (ch >> 3) & 1);
}

inline float readMux(uint8_t sigPin, uint8_t ch) {
  selectMux(ch);
  delayMicroseconds(8);  // 4067 settle
  return (float)analogRead(sigPin) / 4095.0f;
}

void scanControls() {
  for (uint8_t ch = 0; ch < 16; ch++) {
    float v = readMux(PIN_MUX_A_SIG, ch);
    potSmoothA[ch] += (v - potSmoothA[ch]) * 0.2f;
  }
  for (uint8_t ch = 0; ch < 13; ch++) {
    float v = readMux(PIN_MUX_B_SIG, ch);
    potSmoothB[ch] += (v - potSmoothB[ch]) * 0.2f;
  }

  grime::VoiceParams *vp = engine.vp;
  vp[0].pitchKnob = potSmoothA[A_KICK_PITCH];  vp[0].decayKnob = potSmoothA[A_KICK_DECAY];
  vp[0].charKnob  = potSmoothA[A_KICK_CHAR];
  vp[1].pitchKnob = potSmoothA[A_SNARE_PITCH]; vp[1].decayKnob = potSmoothA[A_SNARE_DECAY];
  vp[1].charKnob  = potSmoothA[A_SNARE_CHAR];
  vp[2].pitchKnob = potSmoothA[A_HATCL_PITCH]; vp[2].decayKnob = potSmoothA[A_HATCL_DECAY];
  vp[2].charKnob  = potSmoothA[A_HATCL_CHAR];
  vp[3].pitchKnob = potSmoothA[A_HATOP_PITCH]; vp[3].decayKnob = potSmoothA[A_HATOP_DECAY];
  vp[3].charKnob  = potSmoothA[A_HATOP_CHAR];
  vp[4].pitchKnob = potSmoothB[B_PERC_PITCH];  vp[4].decayKnob = potSmoothB[B_PERC_DECAY];
  vp[4].charKnob  = potSmoothB[B_PERC_CHAR];
  vp[5].pitchKnob = potSmoothB[B_WILD_PITCH];  vp[5].decayKnob = potSmoothB[B_WILD_DECAY];
  vp[5].charKnob  = potSmoothB[B_WILD_CHAR];

  engine.gp.drive       = potSmoothA[A_DRIVE];
  engine.gp.noise       = potSmoothA[A_NOISE];
  engine.gp.master      = potSmoothA[A_MASTER];
  engine.gp.degradeKnob = potSmoothA[A_DEGRADE];

  engine.gp.degradeCV = potSmoothB[B_DEGRADE_CV];
  vp[0].pitchCV = potSmoothB[B_KICK_PCV];
  vp[4].pitchCV = potSmoothB[B_PERC_PCV];
  vp[5].pitchCV = potSmoothB[B_WILD_PCV];
  vp[2].decayCV = potSmoothB[B_HATCL_DCV];
  vp[3].decayCV = potSmoothB[B_HATOP_DCV];
  vp[1].charCV  = potSmoothB[B_SNARE_CCV];

  for (int i = 0; i < 6; i++)
    grime::computeCoefs((grime::VoiceType)i, engine.vs[i], vp[i]);
}

void updateTriggersButtonsLEDs() {
  uint32_t now = millis();
  for (int i = 0; i < 6; i++) {
    // trigger in: rising edge, 2ms lockout
    int t = digitalRead(PIN_TRIG[i]);
    if (t && !trigPrev[i] && (now - trigLock[i] > 2)) {
      engine.trigger(i);
      trigFlash[i] = 1.0f;
      trigLock[i] = now;
    }
    trigPrev[i] = t;

    // mute button: falling edge, 25ms debounce, toggles
    int b = digitalRead(PIN_BTN[i]);
    if (!b && btnPrev[i] && (now - btnDeb[i] > 25)) {
      engine.vp[i].muted = !engine.vp[i].muted;
      btnDeb[i] = now;
    }
    btnPrev[i] = b;

    // illuminated mute button = activity LED
    //   playing: dark at rest, bright flash on trigger decaying with envelope
    //   muted:   dim steady glow, flashes brighter on incoming triggers
    trigFlash[i] *= 0.90f;
    float env = engine.envLevel(i);
    float bri = engine.vp[i].muted
                  ? (0.12f + 0.60f * trigFlash[i])
                  : (env + 0.50f * trigFlash[i]);
    analogWrite(PIN_LED[i], (int)(grime::clampf(bri, 0.0f, 1.0f) * 255.0f));
  }
}

// ---------------------------------------------------------------- setup/loop
void setup() {
  grime::initLUT();
  analogReadResolution(12);

  pinMode(PIN_MUX_S0, OUTPUT); pinMode(PIN_MUX_S1, OUTPUT);
  pinMode(PIN_MUX_S2, OUTPUT); pinMode(PIN_MUX_S3, OUTPUT);
  for (int i = 0; i < 6; i++) {
    pinMode(PIN_TRIG[i], INPUT);
    pinMode(PIN_BTN[i], INPUT_PULLUP);
    pinMode(PIN_LED[i], OUTPUT);
    analogWrite(PIN_LED[i], 0);
  }

  AudioMemory(24);
  codec.enable();
  codec.volume(0.8);
}

void loop() {
  scanControls();
  updateTriggersButtonsLEDs();
}
