from pathlib import Path
from math import cos, pi, sin, sqrt, tan

root = Path(__file__).resolve().parents[1] / "firmware/src"
values = sorted({round(20 * 1000 ** (i / 95)) for i in range(96)} | {100, 350, 2000, 8000})
q28 = 1 << 28
h = [
    "/* SPDX-License-Identifier: GPL-3.0-only */",
    "/* Precomputed at 44.1 kHz. See tools/gen_track_eq.py. */",
    "#define EQ_FREQ_COUNT " + str(len(values)),
]
for label, hz in zip(("LOW", "LOW_MID", "HIGH_MID", "HIGH"), (100, 350, 2000, 8000)):
    h.append(f"#define EQ_FREQ_{label} {values.index(hz)}")
h += [
    "typedef struct {",
    "    uint16_t hz, warp;",
    "    int32_t shelf, band, a1, a2;",
    "} eq_freq_t;",
    "static const eq_freq_t EQ_FREQ[EQ_FREQ_COUNT] = {",
]
for hz in values:
    w = 2 * pi * hz / 44100
    g = tan(w / 2)
    a = sin(w) / sqrt(2)
    values_q28 = [g / (1 + g), a / (1 + a), -2 * cos(w) / (1 + a), (1 - a) / (1 + a)]
    h.append("    {" + ", ".join(str(x) for x in (hz, round(g * 2048), *(round(x * q28) for x in values_q28))) + "},")
h += ["};", "static const int32_t EQ_DELTA[25] = {"]
h.append("    " + ", ".join(str(round((10 ** (db / 20) - 1) * 16384)) for db in range(-12, 13)))
h += ["};", "static const uint32_t EQ_POWER_Q14[97] = {"]
powers = [str(round(10 ** (qdb / 40) * 16384)) for qdb in range(-48, 49)]
for offset in range(0, len(powers), 12):
    h.append("    " + ", ".join(powers[offset:offset+12]) + ",")
h += ["};", ""]
(root / "track_eq_table.h").write_text("\n".join(h), newline="\n")

