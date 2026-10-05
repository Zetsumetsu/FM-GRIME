# FM GRIME — Prototype Build BOM (v0.1)

One module. Ballpark parts cost: **roughly $100–140** before panel/PCB fab
(prices move around — treat as an order-of-magnitude check, not a quote).

Strategy note: the CS42448 is QFP-48 / 0.5mm pitch — it wants a proper fabbed
PCB, not perfboard. Sensible prototype path: fab one main PCB carrying the
Teensy footprint, CS42448 + support, and output buffers; prove the control
side (muxes, conditioning, buttons) on perfboard or a second simple PCB.

## Core

| Qty | Part | Notes |
|-----|------|-------|
| 1 | Teensy 4.1 (with header pins) | MCU, 600 MHz |
| 1 | CS42448-CQZ | 8-ch DAC codec, QFP-48 0.5mm. The quality pick — 7 DACs used (6 voice + main), 1 spare |
| 2 | CD74HC4067 (DIP-24) | 16-ch analog muxes; 29 of 32 channels used |

## Panel controls

| Qty | Part | Notes |
|-----|------|-------|
| 21 | Pot 10kΩ linear, 9mm vertical (Bourns PTV09A / Alpha RD901F or similar) | 18 voice pots + FM DRIVE + NOISE + MASTER |
| 1 | Pot 10kΩ linear, 16mm (Alpha RV16AF or similar) | DEGRADE — the big knob |
| 22 | Knobs to suit | 21 small + 1 large (Davies 1900H-style is the classic) |
| 6 | Illuminated tact button, red LED, 12×12mm | mute + activity LED combined |
| 20 | 3.5mm jack socket (Thonkiconn PJ398SM or similar) | 6 trig in + 7 CV in + 6 indiv out + 1 main out |

## Input conditioning (13 inputs: 6 trigger + 7 CV)

0–5V scaled to 0–3.3V at the ADC, Schottky-clamped for protection.

| Qty | Part | Notes |
|-----|------|-------|
| 13 | 10kΩ resistor, 1/4W | series per input |
| 13 | 20kΩ resistor, 1/4W | shunt per input (divider → 0–5V maps to 0–3.3V) |
| 13 | BAT54S dual Schottky | clamp to 3.3V rail / GND per input |
| 6 | 470Ω resistor, 1/4W | LED current limit for illuminated buttons (tune to taste) |

## Output stage (7 channels)

| Qty | Part | Notes |
|-----|------|-------|
| 2 | TL074 quad op-amp (DIP-14) | buffer the 7 DAC outputs (8th channel spare) |
| 7 | 10µF electrolytic | AC coupling per output |
| 7 | 100Ω resistor, 1/4W | series protection per output |

## Power

| Qty | Part | Notes |
|-----|------|-------|
| 1 | 16-pin (2×8) shrouded IDC box header | Eurorack power in |
| 1 | 16-pin Eurorack ribbon cable | pre-made is fine |
| 1 | 5V DC-DC converter, ≥500mA (Recom R-78E5.0-0.5 or similar) | +12V → 5V for Teensy + codec |
| 4 | 100nF ceramic | rail decoupling |
| 2 | 47µF electrolytic | bulk |

## CS42448 support (on the PCB)

- Decoupling per the datasheet layout guidance: 100nF ceramic on each
  analog supply pin, 10µF bulk nearby. MCLK comes from the Teensy — no
  crystal needed. Follow the datasheet grounding (separate analog/digital
  ground pours, single tie point).

## Misc

| Qty | Part | Notes |
|-----|------|-------|
| 1 | 40-pin breakaway header strip | socket the Teensy |
| 4 | DIP sockets: 2× 24-pin, 2× 14-pin | for the 4067s and TL074s |
| 1 | Micro-USB cable | firmware upload |
| 1 | 32HP Eurorack panel | custom fab from `panel/` artwork |
| — | Perfboard / stripboard | for first-bring-up of the control side (optional) |

Assumes a basic resistor/ceramic-cap kit and hookup wire on hand; the
quantities above are the FM-GRIME-specific adds.
