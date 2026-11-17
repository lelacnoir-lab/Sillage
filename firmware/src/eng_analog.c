/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* ANALOG: two band-limited oscillators (saw / square / tri / sine / PWM),
 * noise, drive and a trapezoidal low-pass.
 * (Skiff 1.3, from Felucca 1.5) WAVE SYNC and SUB (appended: WAVE 0..4 play as they always did; firmware before reads
 * them as PWM) are two more oscillator pairs (analog_render_x):
 *   SYNC  OSC 2 a saw hard-synced to OSC 1 (a saw): it restarts with OSC 1's every cycle, band-limited at the restart
 *         (polyBLEP scaled by the step the restart makes). DTN (shown SYNC) sets OSC 2 above OSC 1, 3/16 semitone a
 *         step (127: +23.8 semitones), and the SHP routings sweep it (ENV DEST / LFO DEST SHP, the matrix's SHP: 3/16
 *         semitone a step of shape, as PWM takes them for its width): the sync sweep. MIX between the two as for
 *         every pair (OSC 2 alone at 127)
 *   SUB   OSC 1 a saw, OSC 2 a square an octave below it (detuned by DTN in cents as before): MIX (shown SUB) is the
 *         sub oscillator's level */
static const char *const N_ANALOG_WAVE[] = {"SAW", "SQR", "TRI", "SIN", "PWM", "SYNC", "SUB"};
enum { AW_SYNC = 5, AW_SUB = 6 };
static const param_desc_t ANALOG_SYNC = {"SYNC", 0, 0, 127, 10, F_INT, 0};   /* DTN's range and default */
static const param_desc_t ANALOG_SUB = {"SUB", 0, 0, 127, 64, F_PCT, 0};      /* MIX's */

static const param_desc_t *analog_desc(const track_t *t, uint32_t k)
{
    if (k == 1u && t->p[P_E0] == AW_SYNC)
        return &ANALOG_SYNC;
    if (k == 2u && t->p[P_E0] == AW_SUB)
        return &ANALOG_SUB;
    return 0;
}

static void analog_note_on(track_t *t, voice_t *v)
{
    (void)t;
    v->ph[1] = v->ph[0] + 0x40000000u;
    v->s[0] = v->s[1] = 0;                            /* filter */
    if (!v->s[2])
        v->s[2] = 0x1234567 + (int32_t)v->age;        /* noise state */
    v->s[3] = 0;                                      /* SYNC: no restart pending */
}

/* WAVE SYNC / SUB: OSC 1 a saw, OSC 2 the synced saw / the sub square; then noise, drive and the filter as
 * analog_render. Its own function: analog_render's loop stays what it was */
static __attribute__((noinline)) void analog_render_x(track_t *t, voice_t *v, int32_t *out, uint32_t n,
                                                      const vmod_t *m)
{
    const int16_t *p = t->p;
    uint32_t sub = p[P_E0] == AW_SUB, i;
    int32_t det = p[P_E1], mix = p[P_E2], noise = p[P_E3];
    int32_t cut = (p[P_E4] << 8) + m->cutoff + (p[P_E7] * (v->pitch16 - 60 * 16) >> 4);
    tsvf_t flt;
    int32_t drive = 32768 + p[P_E6] * 512;
    int32_t m2 = mix * 258, m1 = 32767 - m2;
    int32_t nz = noise * 200, drv = p[P_E6];
    uint32_t ph0 = v->ph[0], ph1 = v->ph[1], inc1 = m->inc, inc2;
    int32_t ic1 = v->s[0], ic2 = v->s[1], nst = v->s[2];
    int32_t jl = v->s[3];                             /* SYNC: 1 + the step of a restart a sample ago (16 bits), 0 none */
    if (sub) {                                        /* an octave below OSC 1, DTN as OSC 2 always had it */
        inc2 = (det ? cents_inc(m->pitch16, det, 0) : inc1) >> 1;
    } else {                                          /* DTN and the shape above OSC 1, 3/16 semitone a step */
        inc2 = pitch_inc(clamp(m->pitch16 + det * 3 + (((m->shape - (64 << 8)) * 3) >> 8), 0, 2047));
        if (m->fine)
            inc2 += (uint32_t)((int32_t)(inc2 >> 12) * m->fine);
    }
    tsvf_coef(&flt, cut, p[P_E5]);
    for (i = 0; i < n; i++) {
        int32_t a = osc_saw(ph0, inc1), b, s;
        if (sub) {
            b = osc_pulse(ph1, inc2, 0x80000000u);
        } else {
            int32_t bl = blep(ph1, inc2);
            if (jl) {                                 /* just restarted: the step was jl - 1, not a whole cycle */
                bl = (bl * ((jl - 1) >> 1)) >> 15;
                jl = 0;
            }
            b = (int32_t)(ph1 >> 16) - 32768 - bl;
            if (ph0 > 0xFFFFFFFFu - inc1)             /* OSC 1 restarts it before the next sample: round that step */
                b -= (blep(ph0, inc1) * (int32_t)(ph1 >> 17)) >> 15;
        }
        ph0 += inc1;
        ph1 += inc2;
        if (!sub && ph0 < inc1) {                     /* OSC 1 wrapped: OSC 2 restarts at the same point */
            uint32_t f = ph0 / ((inc1 >> 16) | 1u);   /* the part of a sample since, 0..65535 */
            uint32_t at = ph1 - f * (inc2 >> 16);     /* OSC 2 at the wrap */
            jl = (int32_t)(at >> 16) + 1;
            ph1 = f * (inc2 >> 16);
        }
        s = mulq15(a, m1) + mulq15(b, m2);
        if (nz)
            s += mulq15((int32_t)(noise32(&nst) >> 17) - 16384, nz);
        if (drv)
            s = softclip(((s >> 2) * (drive >> 2)) >> 11);
        s = soft_knee(tsvf_lp(&flt, s >> 1, &ic1, &ic2), 16000) << 1;
        out[i] += voice_amp2(s, m, i);
    }
    v->ph[0] = ph0;
    v->ph[1] = ph1;
    v->s[0] = ic1;
    v->s[1] = ic2;
    v->s[2] = nst;
    v->s[3] = jl;
}

/* WAVE 0..4: the loop as it always was */
static inline __attribute__((always_inline)) void analog_osc(track_t *t, voice_t *v, int32_t *out, uint32_t n,
                                                             const vmod_t *m)
{
    const int16_t *p = t->p;
    uint32_t wave = (uint32_t)p[P_E0], i;
    int32_t det = p[P_E1], mix = p[P_E2], noise = p[P_E3];
    int32_t cut = (p[P_E4] << 8) + m->cutoff + (p[P_E7] * (v->pitch16 - 60 * 16) >> 4);
    tsvf_t flt;
    int32_t drive = 32768 + p[P_E6] * 512;                       /* 1x .. 3x */
    uint32_t inc1 = m->inc;
    uint32_t inc2 = det ? cents_inc(m->pitch16, det, 0) : inc1;      /* DTN in cents */
    uint32_t pw = 0x80000000u + (uint32_t)((m->shape - (64 << 8)) << 15);
    int32_t m2 = mix * 258, m1 = 32767 - m2;                    /* osc mix Q15 */
    int32_t nz = noise * 200, drv = p[P_E6];
    uint32_t ph0 = v->ph[0], ph1 = v->ph[1];                  /* state in locals: out[] may alias v->s[] */
    int32_t ic1 = v->s[0], ic2 = v->s[1], nst = v->s[2];
    tsvf_coef(&flt, cut, p[P_E5]);
    for (i = 0; i < n; i++) {
        int32_t a, b, s;
        switch (wave) {
        case 1:
            a = osc_pulse(ph0, inc1, 0x80000000u);
            b = osc_pulse(ph1, inc2, 0x80000000u);
            break;
        case 2:
            a = osc_tri(ph0);
            b = osc_tri(ph1);
            break;
        case 3:
            a = osc_sine(ph0);
            b = osc_sine(ph1);
            break;
        case 4:
            a = osc_pulse(ph0, inc1, pw);
            b = osc_pulse(ph1, inc2, pw);
            break;
        default:
            a = osc_saw(ph0, inc1);
            b = osc_saw(ph1, inc2);
            break;
        }
        ph0 += inc1;
        ph1 += inc2;
        s = mulq15(a, m1) + mulq15(b, m2);
        if (nz)
            s += mulq15((int32_t)(noise32(&nst) >> 17) - 16384, nz);
        if (drv)
            s = softclip(((s >> 2) * (drive >> 2)) >> 11);   /* pre-shifts: drive is up to 3x, no overflow */
        {   /* filter, linear up to half scale, then a soft knee (only resonance peaks saturate); soft_knee()
             * written out: the host compiler makes this loop 1 % slower with the call */
            int32_t y = tsvf_lp(&flt, s >> 1, &ic1, &ic2);
            int32_t a = y < 0 ? -y : y;
            if (a > 16000) {
                a = 16000 + (softclip((a - 16000) * 2) >> 1);
                y = y < 0 ? -a : a;
            }
            s = y << 1;
        }
        out[i] += voice_amp2(s, m, i);
    }
    v->ph[0] = ph0;
    v->ph[1] = ph1;
    v->s[0] = ic1;
    v->s[1] = ic2;
    v->s[2] = nst;
}
static void analog_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    if (t->p[P_E0] >= AW_SYNC)
        analog_render_x(t, v, out, n, m);
    else
        analog_osc(t, v, out, n, m);
}

static const preset_t ANALOG_PRESETS[] = {
    {"SAW LEAD", {0, 12, 64, 0, 90, 30, 10, 64}, {4, 70, 100, 50}, 20, 1, FX(0, 10, 45, 30), TAG(PT_LEAD)},
    {"SOFT PAD", {0, 20, 64, 4, 60, 10, 0, 32}, {80, 90, 110, 95}, 10, 0, FX(0, 60, 20, 70), TAG(PT_PAD)},
    {"SQR BASS", {1, 0, 0, 0, 50, 70, 40, 64}, {0, 60, 40, 30}, 30, 1, FX(5, 0, 10, 10), TAG(PT_BASS)},
    {"PWM STR", {4, 8, 40, 0, 75, 20, 0, 48}, {60, 80, 110, 85}, 8, 0, FX(0, 50, 20, 60), TAG(PT_PAD)},
    {"ACID", {0, 0, 0, 0, 50, 100, 25, 64}, {0, 55, 20, 30}, 48, 1, FX(20, 0, 45, 15), TAG(PT_BASS)},
    {"SINE KEY", {3, 6, 50, 0, 127, 0, 0, 0}, {2, 80, 30, 70}, 0, 0, FX(0, 30, 25, 40), TAG(PT_KEYS)},
    {"RAVE", {4, 30, 64, 0, 85, 20, 30, 64}, {20, 80, 110, 60}, 10, 1, FX(30, 40, 30, 30), TAG(PT_LEAD)},
    {"SUB BASS", {3, 0, 0, 0, 40, 0, 20, 0}, {0, 60, 100, 20}, 0, 1, FX(0, 0, 0, 10), TAG(PT_BASS)},
    {"SYNTH PLUCK", {0, 8, 50, 0, 30, 40, 0, 64}, {0, 88, 0, 60}, 55, 0, FX(0, 20, 50, 30), TAG(PT_PLUCK)},
    {"SYNTH BRASS", {0, 10, 64, 0, 45, 20, 10, 64}, {35, 70, 90, 45}, 40, 0, FX(0, 20, 20, 40), TAG(PT_LEAD)},
    {"WIND SWEEP", {0, 0, 0, 90, 30, 90, 0, 0}, {60, 90, 60, 80}, 50, 0, FX(0, 30, 30, 70), TAG(PT_FX)},
    {"STRINGS", {4, 6, 64, 0, 92, 8, 0, 48}, {48, 90, 118, 72}, 6, 0, FX(0, 95, 15, 55), MORE({P_LD_SHP, 20}, {P_LRATE, 72}, {P_LD_PIT, 8}, {P_LFADE, 80}), TAG(PT_PAD)},   /* ((Skiff phase 9) as SOFT PAD to the ear: a bright string
                                                                         * machine now, its PWM moving, a late vibrato) */
    /* (Skiff) for the themes: CRYPT (dungeon synth), NEON (synthwave), QUARK (particles) */
    {"BONE FLUTE", {2, 3, 30, 22, 84, 15, 0, 64}, {56, 80, 110, 64}, 6, 1, FX(0, 15, 30, 85), MORE({P_LRATE, 84}, {P_LFADE, 90}, {P_LD_PIT, 8}), TAG(PT_LEAD)},
    {"WAR HORN", {0, 5, 64, 4, 42, 12, 20, 64}, {72, 80, 110, 80}, 30, 1, FX(0, 10, 10, 90), MORE({P_LRATE, 80}, {P_LFADE, 96}, {P_LD_PIT, 8}), TAG(PT_LEAD)},
    {"DARK STRINGS", {1, 4, 64, 6, 40, 34, 22, 40}, {100, 90, 120, 105}, 10, 0, FX(0, 30, 0, 110), MORE({P_LRATE, 30}, {P_LD_FLT, 8}, {P_LD_PIT, 8}, {P_LFADE, 110}), TAG(PT_PAD)},   /* ((phase 9) hollow and dark, a slow swell) */
    {"NIGHT BASS", {0, 7, 64, 0, 40, 35, 15, 64}, {0, 64, 50, 30}, 45, 1, FX(0, 5, 10, 8), TAG(PT_BASS)},
    {"NEON LEAD", {0, 14, 64, 0, 92, 25, 12, 64}, {6, 70, 110, 55}, 14, 1, FX(0, 25, 55, 40), MORE({P_LRATE, 86}, {P_LFADE, 92}, {P_LD_PIT, 11}, {P_GLIDE, 28}), TAG(PT_LEAD)},
    {"SUNSET PAD", {4, 9, 55, 0, 74, 15, 0, 48}, {78, 90, 115, 92}, 10, 0, FX(0, 85, 25, 70), MORE({P_LWAVE, 1}, {P_LRATE, 52}, {P_LD_SHP, 48}), TAG(PT_PAD)},
    {"HERO BRASS", {0, 16, 64, 0, 50, 18, 8, 56}, {50, 75, 95, 55}, 44, 0, FX(0, 45, 20, 45), TAG(PT_PAD)},
    {"SYN TOM", {3, 0, 0, 6, 127, 0, 25, 0}, {0, 76, 0, 60}, 0, 0, FX(0, 0, 10, 45), MORE({P_ED_PIT, 63}), TAG(PT_DRUM)},
    {"QUANTA", {3, 0, 0, 0, 127, 0, 0, 0}, {0, 64, 127, 50}, 0, 1, FX(0, 0, 55, 35), MORE({P_LWAVE, 4}, {P_LRATE, 96}, {P_LD_PIT, 50}), TAG(PT_FX)},
    /* (Skiff phase 9) more for every engine: basses, kits */
    {"REESE", {0, 40, 64, 0, 52, 15, 30, 32}, {0, 64, 127, 30}, 10, 1, FX(10, 20, 5, 10), MORE({P_LRATE, 30}, {P_LD_FLT, 8}), TAG(PT_BASS)},
    {"DRIVE BASS", {0, 14, 64, 0, 62, 20, 10, 64}, {0, 64, 64, 25}, 24, 1, FX(40, 0, 5, 5), MORE({P_DTYPE, 1}, {P_DTONE, -32}), TAG(PT_BASS)},
    /* (1.3) WAVE SYNC: the envelope sweeps the synced saw (ENV DEST SHAPE); SUB: a saw on its square an octave down */
    {"HARD SYNC", {5, 36, 96, 0, 88, 24, 12, 64}, {2, 70, 96, 40}, 16, 1, FX(0, 15, 40, 30), MORE({P_ED_SHP, 44}, {P_GLIDE, 20}), TAG(PT_LEAD)},
    {"SUB SAW", {6, 0, 80, 0, 70, 22, 6, 64}, {0, 60, 110, 22}, 20, 1, FX(0, 0, 5, 10), TAG(PT_BASS)},
    /* Sillage vintage bank: Jupiter and Juno basses, then Oberheim and Prophet pads. Append to keep saved IDs. */
    {"ORBIT SAW", {0, 11, 55, 0, 49, 34, 12, 56}, {0, 53, 83, 26}, 36, 1,
        FX(5, 0, 0, 4), TAG(PT_BASS)},
    {"ORBIT SYNC", {AW_SYNC, 35, 87, 0, 68, 22, 16, 52}, {1, 48, 62, 24}, 20, 1,
        FX(8, 0, 0, 5), MORE({P_ED_SHP, 34}, {P_GLIDE, 12}), TAG(PT_BASS)},
    {"PULSE SUB", {AW_SUB, 0, 100, 0, 40, 15, 5, 44}, {0, 67, 112, 18}, 26, 1,
        FX(0, 0, 0, 0), TAG(PT_BASS)},
    {"PULSE RUBBER", {4, 0, 0, 0, 57, 50, 4, 50}, {0, 51, 44, 24}, 40, 1,
        FX(0, 10, 0, 4), MORE({P_ED_SHP, 24}), TAG(PT_BASS)},
    {"VELVET BRASS", {0, 18, 61, 0, 55, 13, 3, 52}, {42, 69, 89, 73}, 39, 0,
        FX(0, 10, 0, 33), MORE({P_SPRD, 62}), TAG(PT_PAD)},
    {"VELVET SKY", {0, 23, 65, 0, 66, 10, 0, 54}, {84, 94, 108, 105}, 14, 0,
        FX(0, 31, 0, 53), MORE({P_LRATE, 34}, {P_LD_FLT, 10}, {P_SPRD, 74}), TAG(PT_PAD)},
    {"EMBER CHORD", {1, 7, 48, 0, 48, 31, 7, 59}, {27, 80, 81, 65}, 35, 0,
        FX(0, 7, 0, 23), TAG(PT_PAD)},
    {"EMBER SWELL", {4, 10, 50, 3, 44, 29, 0, 65}, {98, 91, 111, 96}, 26, 0,
        FX(0, 14, 0, 40), MORE({P_LRATE, 41}, {P_LD_SHP, 17}), TAG(PT_PAD)},
    /* Sillage: basses from sub and pulse to saw and sync. */
    {"DEEP PULSE", {AW_SUB, 3, 115, 0, 46, 9, 5, 42}, {2, 66, 95, 24}, 21, 1, FX(0, 0, 0, 3), TAG(PT_BASS)},
    {"ROUND SUB", {3, 0, 0, 0, 47, 8, 7, 52}, {4, 71, 94, 31}, 9, 1, FX(0, 0, 0, 0), TAG(PT_BASS)},
    {"PULSE SNAP", {4, 2, 35, 1, 57, 43, 7, 56}, {0, 38, 49, 20}, 45, 1, FX(0, 0, 0, 3),
        MORE({P_ED_SHP, 18}), TAG(PT_BASS)},
    {"RUBBER PWM", {4, 7, 77, 0, 59, 34, 10, 56}, {1, 70, 63, 34}, 33, 1, FX(0, 9, 0, 7),
        MORE({P_LRATE, 54}, {P_LD_SHP, 23}), TAG(PT_BASS)},
    {"DUAL SAW BS", {0, 24, 74, 0, 51, 19, 11, 48}, {1, 56, 92, 24}, 35, 1, FX(4, 0, 0, 6), TAG(PT_BASS)},
    {"SYNC BITE", {AW_SYNC, 25, 104, 0, 65, 41, 12, 56}, {0, 46, 68, 21}, 31, 1, FX(5, 0, 0, 4),
        MORE({P_ED_SHP, 49}), TAG(PT_BASS)},
    /* Sillage: mono leads and articulated poly leads. */
    {"GLIDE SAW", {0, 14, 51, 0, 83, 23, 9, 63}, {5, 60, 103, 46}, 15, 1, FX(0, 16, 29, 24),
        MORE({P_GLIDE, 34}), TAG(PT_LEAD)},
    {"PULSE SOLO", {4, 6, 75, 0, 88, 37, 6, 58}, {4, 49, 91, 41}, 21, 1, FX(0, 7, 21, 17),
        MORE({P_LRATE, 75}, {P_LD_SHP, 19}), TAG(PT_LEAD)},
    {"SYNC EDGE", {AW_SYNC, 47, 99, 0, 82, 16, 10, 61}, {3, 69, 94, 32}, 27, 1, FX(3, 4, 18, 19),
        MORE({P_ED_SHP, 39}), TAG(PT_LEAD)},
    {"COPPER HORN", {0, 16, 57, 1, 65, 22, 8, 53}, {17, 75, 90, 46}, 39, 0, FX(0, 8, 12, 29), TAG(PT_LEAD)},
    {"TRI WHISTLE", {2, 0, 0, 3, 89, 44, 0, 72}, {26, 56, 107, 56}, 12, 1, FX(0, 0, 24, 35),
        MORE({P_LRATE, 78}, {P_LD_PIT, 6}, {P_LFADE, 69}), TAG(PT_LEAD)},
    /* Sillage: brass, string ensemble and filter swells. */
    {"BRASS DRIFT", {0, 13, 64, 2, 49, 15, 4, 54}, {56, 67, 93, 72}, 43, 0, FX(0, 13, 0, 35),
        MORE({P_LRATE, 29}, {P_LD_FLT, 9}, {P_SPRD, 52}), TAG(PT_PAD)},
    {"WIDE STRINGS", {4, 19, 68, 0, 77, 11, 2, 54}, {73, 89, 114, 98}, 14, 0, FX(0, 65, 0, 41),
        MORE({P_LRATE, 53}, {P_LD_SHP, 24}, {P_SPRD, 68}), TAG(PT_PAD)},
    {"EMBER VEIL", {1, 11, 54, 1, 40, 23, 6, 62}, {70, 86, 94, 82}, 30, 0, FX(0, 18, 0, 33), TAG(PT_PAD)},
    {"AMBER MOTION", {4, 12, 54, 4, 59, 38, 0, 51}, {96, 91, 107, 105}, 31, 0, FX(0, 19, 0, 39),
        MORE({P_LWAVE, 1}, {P_LRATE, 31}, {P_LD_FLT, 20}, {P_LD_SHP, 16}), TAG(PT_PAD)},
    {"DUSK STRINGS", {2, 20, 74, 8, 64, 13, 4, 54}, {83, 83, 117, 103}, 23, 0, FX(0, 30, 0, 54),
        MORE({P_SPRD, 65}), TAG(PT_PAD)},
    /* Sillage: keys with a useful sustain or a responsive struck envelope. */
    {"PULSE KEYS", {4, 6, 60, 0, 57, 22, 4, 62}, {1, 64, 61, 45}, 39, 0, FX(0, 15, 0, 18), TAG(PT_KEYS)},
    {"WARM POLY", {0, 13, 58, 0, 51, 17, 4, 59}, {3, 72, 70, 47}, 31, 0, FX(0, 12, 0, 20), TAG(PT_KEYS)},
    {"REED CHORD", {1, 9, 51, 0, 66, 40, 9, 65}, {5, 54, 86, 33}, 25, 0, FX(0, 7, 0, 13), TAG(PT_KEYS)},
    {"VELVET ORGAN", {2, 4, 41, 1, 100, 12, 6, 68}, {5, 62, 121, 28}, 7, 0, FX(0, 28, 0, 19), TAG(PT_KEYS)},
    {"CHORUS PULSE", {4, 7, 55, 0, 66, 11, 0, 66}, {3, 78, 99, 37}, 12, 0, FX(0, 59, 0, 19),
        MORE({P_LRATE, 45}, {P_LD_SHP, 16}), TAG(PT_KEYS)},
    {"SOFT MALLET", {2, 5, 36, 4, 77, 24, 2, 67}, {0, 80, 18, 43}, 20, 0, FX(0, 5, 9, 28), TAG(PT_KEYS)},
    /* Sillage: round, hollow, resonant and sync plucks. */
    {"SOFT PICK", {2, 3, 35, 0, 56, 21, 1, 62}, {0, 55, 0, 29}, 34, 0, FX(0, 4, 0, 16), TAG(PT_PLUCK)},
    {"SINE PIN", {3, 0, 0, 0, 120, 0, 6, 64}, {0, 46, 0, 25}, 0, 0, FX(0, 0, 15, 15), TAG(PT_PLUCK)},
    {"MUTED STRING", {1, 3, 37, 2, 47, 28, 4, 63}, {0, 39, 0, 22}, 46, 0, FX(0, 0, 0, 10), TAG(PT_PLUCK)},
    {"HOLLOW PLUCK", {4, 2, 52, 0, 62, 36, 3, 65}, {0, 55, 0, 35}, 41, 0, FX(0, 6, 8, 19),
        MORE({P_ED_SHP, 20}), TAG(PT_PLUCK)},
    {"PULSE TICK", {4, 0, 26, 1, 73, 29, 8, 69}, {0, 32, 0, 18}, 34, 0, FX(0, 0, 11, 10), TAG(PT_PLUCK)},
    {"COPPER PICK", {0, 6, 53, 1, 48, 29, 12, 63}, {1, 59, 2, 32}, 52, 0, FX(0, 3, 0, 22), TAG(PT_PLUCK)},
    {"SAW DROPS", {0, 12, 65, 0, 62, 31, 5, 56}, {0, 57, 0, 42}, 35, 0, FX(0, 12, 32, 21), TAG(PT_PLUCK)},
    {"WIDE PICK", {0, 27, 64, 0, 63, 25, 0, 54}, {1, 65, 0, 50}, 34, 0, FX(0, 27, 7, 28),
        MORE({P_SPRD, 62}), TAG(PT_PLUCK)},
    {"FILTER DROP", {1, 7, 45, 1, 37, 70, 5, 56}, {0, 68, 0, 31}, 57, 0, FX(0, 0, 15, 16), TAG(PT_PLUCK)},
    {"ACID PICK", {0, 2, 18, 0, 51, 85, 18, 56}, {0, 45, 0, 22}, 46, 0, FX(5, 0, 9, 8), TAG(PT_PLUCK)},
    {"SYNC CHIME", {AW_SYNC, 38, 102, 0, 84, 22, 4, 64}, {0, 65, 0, 40}, 25, 0, FX(0, 10, 18, 30),
        MORE({P_ED_SHP, 28}), TAG(PT_PLUCK)},
    {"SYNC STAB", {AW_SYNC, 56, 89, 0, 62, 36, 10, 56}, {0, 37, 0, 26}, 44, 0, FX(0, 3, 4, 14),
        MORE({P_ED_SHP, 49}), TAG(PT_PLUCK)},
    /* Sillage: playable FX patches, from air and sweeps to impacts and glitches. */
    {"DUST RISE", {3, 0, 0, 113, 65, 45, 1, 0}, {104, 77, 95, 77}, 35, 1, FX(0, 10, 0, 37), TAG(PT_FX)},
    {"AIR RUSH", {2, 0, 0, 121, 83, 24, 0, 0}, {31, 79, 8, 65}, 38, 1, FX(0, 0, 7, 41), TAG(PT_FX)},
    {"FOG SWEEP", {3, 0, 0, 105, 45, 62, 0, 0}, {86, 86, 88, 106}, 49, 1, FX(0, 17, 0, 62), TAG(PT_FX)},
    {"WAVE BREAK", {4, 7, 43, 85, 89, 28, 2, 15}, {47, 88, 10, 82}, 26, 1, FX(0, 12, 17, 43), TAG(PT_FX)},
    {"STATIC TIDE", {1, 0, 0, 103, 73, 33, 5, 25}, {25, 64, 101, 61}, 12, 1, FX(0, 0, 20, 28),
        MORE({P_LWAVE, 4}, {P_LRATE, 64}, {P_LD_FLT, 31}), TAG(PT_FX)},
    {"SUB DROP", {3, 0, 0, 0, 66, 8, 7, 64}, {1, 87, 0, 64}, 0, 1, FX(0, 0, 0, 13),
        MORE({P_ED_PIT, 58}), TAG(PT_FX)},
    {"SUB IMPACT", {AW_SUB, 0, 71, 8, 55, 15, 12, 54}, {0, 80, 0, 47}, 18, 1, FX(0, 0, 0, 29),
        MORE({P_ED_PIT, 46}), TAG(PT_FX)},
    {"DUB SIREN", {1, 0, 0, 0, 63, 41, 4, 60}, {14, 69, 96, 53}, 17, 1, FX(0, 0, 50, 22),
        MORE({P_LWAVE, 1}, {P_LRATE, 42}, {P_LD_PIT, 29}), TAG(PT_FX)},
    {"TAPE DIVE", {2, 10, 60, 7, 63, 21, 6, 52}, {62, 95, 48, 78}, 32, 1, FX(0, 17, 22, 32),
        MORE({P_ED_PIT, 53}, {P_GLIDE, 44}), TAG(PT_FX)},
    {"LASER SNAP", {AW_SYNC, 41, 105, 0, 87, 29, 8, 56}, {0, 46, 0, 42}, 24, 1, FX(0, 0, 26, 29),
        MORE({P_ED_PIT, 56}, {P_ED_SHP, 36}), TAG(PT_FX)},
    {"RISING SYNC", {AW_SYNC, 14, 112, 0, 75, 16, 3, 58}, {101, 70, 101, 58}, 22, 1, FX(0, 8, 25, 30),
        MORE({P_ED_SHP, 55}), TAG(PT_FX)},
    {"RADIO STEP", {1, 5, 51, 19, 78, 59, 8, 58}, {3, 63, 93, 36}, 19, 1, FX(0, 0, 21, 22),
        MORE({P_LWAVE, 4}, {P_LRATE, 78}, {P_LD_PIT, 31}), TAG(PT_FX)},
    {"PULSE GATE", {4, 3, 73, 4, 75, 32, 8, 60}, {4, 70, 115, 30}, 18, 1, FX(0, 8, 19, 23),
        MORE({P_LWAVE, 3}, {P_LRATE, 85}, {P_LD_AMP, 63}), TAG(PT_FX)},
    {"FILTER ALARM", {0, 18, 61, 4, 57, 83, 5, 49}, {10, 69, 105, 49}, 22, 1, FX(0, 0, 25, 17),
        MORE({P_LWAVE, 1}, {P_LRATE, 63}, {P_LD_FLT, 41}), TAG(PT_FX)},
    {"SLOW BEACON", {2, 4, 51, 0, 76, 14, 0, 60}, {19, 85, 92, 74}, 8, 1, FX(0, 15, 32, 37),
        MORE({P_LRATE, 38}, {P_LD_AMP, 42}), TAG(PT_FX)},
    {"GHOST SIGNAL", {3, 9, 55, 15, 85, 18, 3, 62}, {87, 69, 97, 104}, 8, 1, FX(0, 32, 24, 65),
        MORE({P_LWAVE, 4}, {P_LRATE, 30}, {P_LD_PIT, 12}), TAG(PT_FX)},
    {"RING WIND", {AW_SYNC, 73, 72, 65, 69, 46, 5, 36}, {76, 81, 105, 90}, 22, 1, FX(0, 12, 0, 52),
        MORE({P_LRATE, 36}, {P_LD_SHP, 36}), TAG(PT_FX)},
    /* Sillage: new sounds for the families previously filled by LOFI. */
    {"LOW TRI", {2, 1, 24, 0, 71, 18, 6, 56}, {0, 60, 84, 32}, 24, 1, FX(0, 0, 0, 8), TAG(PT_BASS)},
    {"LEAD BRASS", {0, 9, 57, 0, 71, 19, 12, 61}, {18, 60, 99, 33}, 44, 1, FX(0, 12, 9, 24), TAG(PT_LEAD)},
    {"RIBBON LEAD", {4, 11, 62, 0, 73, 18, 7, 66}, {7, 68, 94, 49}, 17, 1, FX(0, 15, 24, 25),
        MORE({P_GLIDE, 33}, {P_LRATE, 63}, {P_LD_SHP, 14}), TAG(PT_LEAD)},
    {"DRY PULSE", {1, 0, 0, 0, 102, 31, 4, 76}, {1, 32, 85, 18}, 8, 1, FX(0, 0, 0, 6), TAG(PT_LEAD)},
    {"CEDAR PLUCK", {2, 5, 32, 3, 61, 24, 4, 72}, {0, 71, 0, 38}, 34, 0, FX(0, 5, 6, 21), TAG(PT_PLUCK)},
    /* Sillage expansion bank. */
    {"VA SUB WEIGH", {0, 4, 49, 0, 56, 43, 4, 69}, {0, 40, 100, 13}, 45, 1, FX(0, 0, 0, 0), TAG(PT_BASS)},
    {"VA RUBBER PU", {1, 14, 78, 0, 39, 43, 4, 69}, {1, 72, 70, 22}, 45, 1, FX(0, 0, 0, 0), MORE({P_GLIDE, 16}), TAG(PT_BASS)},
    {"VA SOLO BRAS", {5, 4, 49, 0, 84, 21, 4, 69}, {5, 63, 100, 31}, 25, 1, FX(0, 0, 0, 0), TAG(PT_LEAD)},
    {"VA SLOW BRAS", {4, 4, 49, 0, 67, 21, 4, 69}, {44, 77, 108, 72}, 12, 0, FX(0, 9, 0, 7), TAG(PT_PAD)},
    {"VA PWM VELVE", {0, 14, 78, 0, 50, 21, 4, 69}, {66, 85, 112, 88}, 12, 0, FX(0, 11, 0, 10), TAG(PT_PAD)},
    {"VA DUSK CHOR", {1, 27, 62, 0, 90, 21, 4, 69}, {82, 96, 120, 102}, 12, 0, FX(0, 13, 0, 13), MORE({P_LRATE, 43}, {P_LD_PIT, 4}), TAG(PT_PAD)},
    {"VA DRY TINES", {3, 4, 49, 0, 85, 21, 4, 69}, {0, 76, 36, 38}, 22, 0, FX(0, 0, 0, 0), TAG(PT_KEYS)},
    {"VA WOOD PICK", {2, 4, 49, 0, 74, 21, 4, 69}, {0, 49, 0, 29}, 45, 0, FX(0, 0, 0, 0), TAG(PT_PLUCK)},
    {"VA DUB ALARM", {5, 4, 49, 0, 48, 21, 4, 69}, {4, 74, 86, 42}, 54, 0, FX(0, 0, 0, 9), TAG(PT_FX)},
};

static const engine_t ENG_ANALOG = {
    .name = "ANALOG",
    .page_title = {"OSC", "FLT"},
    .edit = {
        {"WAVE", N_ANALOG_WAVE, 0, 6, 0, F_ENUM, 0},   /* ((1.3) SYNC SUB appended) */
        {"DETUNE", 0, 0, 127, 10, F_INT, U_CT},
        {"MIX", 0, 0, 127, 64, F_PCT, 0},
        {"NOISE", 0, 0, 127, 0, F_PCT, 0},
        {"CUTOFF", 0, 0, 127, 90, F_CUTOFF, 0},
        {"RES", 0, 0, 127, 30, F_PCT, 0},
        {"DRIVE", 0, 0, 127, 0, F_PCT, 0},
        {"KEYTRK", 0, 0, 127, 64, F_PCT, 0},
    },
    .presets = ANALOG_PRESETS,
    .npresets = NELEM(ANALOG_PRESETS),
    .note_on = analog_note_on,
    .render = analog_render,
    .desc = analog_desc,
    .knob = {P_E4, P_E5, P_ATK, P_REL},
    .keep = 0x03,                /* the filter */
};
