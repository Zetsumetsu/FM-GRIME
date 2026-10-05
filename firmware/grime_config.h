// grime_config.h — FM GRIME hardware mapping (Teensy 4.1)
//
// Audio-reserved pins — DO NOT reuse for GPIO:
//   7 = TDM TX, 8 = TDM RX, 20 = TDM LRCLK, 21 = TDM BCLK, 23 = TDM MCLK
//
// Analog conditioning (hardware, on the module PCB):
//   - Trigger ins: divider + clamp to 3.3V, rising-edge detected
//   - CV ins: scaled to 0–3.3V at the ADC
//     - Pitch CV: 0–5V range expected → 1V/oct
//     - Decay/Char/Degrade CV: 0–5V unipolar
#pragma once

#include <Arduino.h>

// ---- digital I/O ----
static const uint8_t PIN_TRIG[6] = {0, 1, 2, 3, 4, 5};        // trigger inputs (conditioned)
static const uint8_t PIN_BTN[6]  = {6, 13, 26, 27, 28, 29};   // mute buttons → GND, INPUT_PULLUP
static const uint8_t PIN_LED[6]  = {9, 10, 11, 12, 24, 25};   // illuminated mute buttons (PWM)

// ---- analog muxes: 2x CD74HC4067, shared select lines ----
#define PIN_MUX_S0 30
#define PIN_MUX_S1 31
#define PIN_MUX_S2 32
#define PIN_MUX_S3 33
#define PIN_MUX_A_SIG A0
#define PIN_MUX_B_SIG A1

// Mux A: 16 pots (12 voice + 4 global)
enum MuxACh {
  A_KICK_PITCH = 0, A_KICK_DECAY = 1, A_KICK_CHAR = 2,
  A_SNARE_PITCH = 3, A_SNARE_DECAY = 4, A_SNARE_CHAR = 5,
  A_HATCL_PITCH = 6, A_HATCL_DECAY = 7, A_HATCL_CHAR = 8,
  A_HATOP_PITCH = 9, A_HATOP_DECAY = 10, A_HATOP_CHAR = 11,
  A_DRIVE = 12, A_NOISE = 13, A_MASTER = 14, A_DEGRADE = 15
};

// Mux B: 6 pots + 7 CV (3 spare)
enum MuxBCh {
  B_PERC_PITCH = 0, B_PERC_DECAY = 1, B_PERC_CHAR = 2,
  B_WILD_PITCH = 3, B_WILD_DECAY = 4, B_WILD_CHAR = 5,
  B_DEGRADE_CV = 6,
  B_KICK_PCV = 7, B_PERC_PCV = 8, B_WILD_PCV = 9,      // 1V/oct pitch CV
  B_HATCL_DCV = 10, B_HATOP_DCV = 11,                  // 0–5V decay CV
  B_SNARE_CCV = 12                                    // 0–5V character CV
};

// ---- defaults ----
#define DEFAULT_NOISE   0.35f
#define DEFAULT_MASTER  0.80f
#define DEGRADE_CV_DEPTH 0.50f   // fixed depth: full CV swing = half the knob range

// ---- TDM output routing (CS42448 DAC channels) ----
// ch 0-5: individual voice outs (pre-degrade), ch 6: main (post-degrade), ch 7: spare (silent)
#define TDM_CH_MAIN 6
