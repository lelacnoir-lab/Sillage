/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* FM6: classic 6-operator FM. The synthesis is msfa (Dexed's core), ported to integer C in fm6_core.c
 * (Apache-2.0); this file is the Felucca engine around it.
 *
 * The patch is the sound: every track has one (fm6_patch, the generic 155-byte single-voice layout: six
 * operators with their 4-rate / 4-level envelopes, keyboard level and rate scaling, velocity, ratio or
 * fixed frequency, detune; 32 algorithms, feedback, LFO, pitch envelope, transpose). It is edited in the
 * web editor (EDITOR_PROTOCOL.md FM6_*: a .syx voice sent to a track), loaded from the factory slots, and saved inside
 * projects (FUN8) and user presets (up_fm6.c). Whatever is loaded becomes the track's patch.
 * On the device the eight EDIT values are macros on top of it:
 *   ALG   PAT = the patch's algorithm, 1..32 another one
 *   FB    added to the patch's feedback (0..7)
 *   MLVL  the output level of every operator that is not a carrier (-36 .. +36 dB): the brightness
 *   MRAT  added to the coarse ratio of the modulators (ratio mode only)
 *   MEG   the modulators' envelope times: + slower (rates down to 40 steps), - faster
 *   VMOD  added to the modulators' velocity sensitivity (0..7)
 *   DTUN  spreads the carriers apart in pitch (up to about +-36 cents between the outer ones)
 *   SLOT  F1..F8 load a factory patch; OWN (8) is the track's own patch: what a project, a user preset or the
 *         editor put there. A load shows F n when the patch is that factory one unchanged, else OWN (fm6_adopt).
 *         Turning SLOT from OWN to F n keeps the own patch aside; back to OWN brings it back (fm6_poll).
 *         Before 1.0.3 the values 8..34 were F1..F8 then B1..B27, a flash bank (retired: its patches moved into
 *         the user presets, up_fm6.c); a stored 8..34 is clamped to 8, OWN, and its patch comes with the record
 * The operator envelopes are the voice's amplitude and end it (engine_t.ownenv / done): the track's ADSR,
 * ENV DEST and the matrix's ENV do nothing here. The track's FLT moves MLVL (ENV / LFO -> FLT, the
 * matrix's CUT), SHP the feedback, PIT the pitch (bend, glide, tune: the voice's pitch and fine factor).
 * Six voices per part at most (engine_t.poly).
 *
 * State: per voice an fm6_note_t (244 bytes, in the part's shared engine area as PHYS's slots: engines.c eng_mem;
 * voice_t.s[] is too small), per part the patch, the patch through the macros (fm6_eff, rebuilt in the audio ISR
 * when either changes) and the LFO (once a block). */
#include "fm6_core.c"
#include "felucca_fm6.h"         /* tools/gen_fm6_patches.py: FM6_INIT, FM6_FACTORY[] */

#define ENGI_FM6 12u             /* engines.c ENGINES[] (append-only) */
#define FM6_POLY 6               /* engine_t.poly */
#define FM6_OWN FM6_NFACTORY     /* SLOT: the track's own patch (F1..F8 before it) */
#define FM6_PACKED 128u

static uint8_t fm6_patch[NTRK][FP_SIZE + 1u];   /* the tracks' patches (main loop writes, then fm6_pgen) */
static volatile uint8_t fm6_pgen[NTRK];          /* +1 after each write of fm6_patch[t] */
static uint8_t fm6_slot[NTRK];                   /* the SLOT value last loaded (main loop); 0xFF = none */
static uint8_t fm6_own[NTRK][FM6_PACKED];        /* the own patch while SLOT shows a factory one (fm6_poll) */
static uint8_t fm6_own_ok;                       /* bit tr: fm6_own[tr] holds it */
static struct {                                  /* the patch through the macros: the audio ISR's copy */
    uint8_t p[FP_SIZE + 1u];
    uint8_t gen, alg, fb, ok;
    int16_t e[7];                                /* P_E0..P_E6 it was made with */
    int32_t dt[6];                               /* DTUN: per operator, Q24 log2 */
} fm6_eff[NTRK];
static fm6_lfo_t fm6_lfo[NTRK];
static int32_t fm6_lfo_v[NTRK], fm6_lfo_d[NTRK]; /* this block's LFO value and delay (Q24) */

/* ------------------------------------------------------- patch formats --- */
/* the highest value of each byte of the 155-byte voice */
static uint32_t fm6_max(uint32_t i)
{
    static const uint8_t OPMAX[21] = {99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 3, 3, 7, 3, 7, 99, 1, 31, 99, 14};
    static const uint8_t VMAX[19] = {99, 99, 99, 99, 99, 99, 99, 99, 31, 7, 1, 99, 99, 99, 99, 1, 5, 7, 48};
    if (i < 126u)
        return OPMAX[i % 21u];
    if (i < FP_NAME)
        return VMAX[i - 126u];
    return 126u;
}

/* every value inside its range (a name byte outside 32..126 becomes a space) */
static void fm6_sanitize(uint8_t *v)
{
    uint32_t i;
    for (i = 0; i < FP_SIZE; i++) {
        if (i >= FP_NAME)
            v[i] = v[i] < 32u || v[i] > 126u ? ' ' : v[i];
        else if (v[i] > fm6_max(i))
            v[i] = (uint8_t)fm6_max(i);
    }
    v[FP_SIZE] = 0;
}

/* 128-byte packed record (the 32-voice bank's) -> 155-byte voice; bits a record does not use are ignored */
static void fm6_unpack(const uint8_t *b, uint8_t *v)
{
    uint32_t k, i;
    for (k = 0; k < 6u; k++) {
        const uint8_t *o = b + k * 17u;
        uint8_t *d = v + k * FP_OP;
        for (i = 0; i < 11u; i++)
            d[i] = o[i] & 0x7Fu;
        d[FP_LC] = o[11] & 3u;
        d[FP_RC] = (o[11] >> 2) & 3u;
        d[FP_RS] = o[12] & 7u;
        d[FP_DET] = (o[12] >> 3) & 15u;
        d[FP_AMS] = o[13] & 3u;
        d[FP_KVS] = (o[13] >> 2) & 7u;
        d[FP_OL] = o[14] & 0x7Fu;
        d[FP_MODE] = o[15] & 1u;
        d[FP_FC] = (o[15] >> 1) & 31u;
        d[FP_FF] = o[16] & 0x7Fu;
    }
    for (i = 0; i < 9u; i++)
        v[FP_PR1 + i] = b[102 + i] & 0x7Fu;              /* pitch EG, algorithm */
    v[FP_ALG] &= 31u;
    v[FP_FB] = b[111] & 7u;
    v[FP_OKS] = (b[111] >> 3) & 1u;
    for (i = 0; i < 4u; i++)
        v[FP_LFS + i] = b[112 + i] & 0x7Fu;
    v[FP_LKS] = b[116] & 1u;
    v[FP_LFW] = (b[116] >> 1) & 7u;
    v[FP_LPMS] = (b[116] >> 4) & 7u;
    v[FP_TRNSP] = b[117] & 0x7Fu;
    for (i = 0; i < 10u; i++)
        v[FP_NAME + i] = b[118 + i] & 0x7Fu;
    fm6_sanitize(v);
}

/* 155-byte voice -> 128-byte packed record (7-bit bytes: it travels in SysEx as it is) */
static void fm6_pack(const uint8_t *v, uint8_t *b)
{
    uint32_t k, i;
    for (k = 0; k < 6u; k++) {
        const uint8_t *o = v + k * FP_OP;
        uint8_t *d = b + k * 17u;
        for (i = 0; i < 11u; i++)
            d[i] = o[i] & 0x7Fu;
        d[11] = (uint8_t)((o[FP_LC] & 3u) | (o[FP_RC] & 3u) << 2);
        d[12] = (uint8_t)((o[FP_RS] & 7u) | (o[FP_DET] & 15u) << 3);
        d[13] = (uint8_t)((o[FP_AMS] & 3u) | (o[FP_KVS] & 7u) << 2);
        d[14] = o[FP_OL] & 0x7Fu;
        d[15] = (uint8_t)((o[FP_MODE] & 1u) | (o[FP_FC] & 31u) << 1);
        d[16] = o[FP_FF] & 0x7Fu;
    }
    for (i = 0; i < 9u; i++)
        b[102 + i] = v[FP_PR1 + i] & 0x7Fu;
    b[110] &= 31u;
    b[111] = (uint8_t)((v[FP_FB] & 7u) | (v[FP_OKS] & 1u) << 3);
    for (i = 0; i < 4u; i++)
        b[112 + i] = v[FP_LFS + i] & 0x7Fu;
    b[116] = (uint8_t)((v[FP_LKS] & 1u) | (v[FP_LFW] & 7u) << 1 | (v[FP_LPMS] & 7u) << 4);
    b[117] = v[FP_TRNSP] & 0x7Fu;
    for (i = 0; i < 10u; i++)
        b[118 + i] = v[FP_NAME + i] & 0x7Fu;
}

/* ---------------------------------------------------- the track's patch --- */
/* track tr's patch = v (155 bytes, sanitized). Main loop: the ISR takes it at its next block */
static void fm6_set_patch(uint32_t tr, const uint8_t *v)
{
    uint8_t s[FP_SIZE + 1u];
    tr %= NTRK;
    memcpy(s, v, FP_SIZE);
    fm6_sanitize(s);                                /* not in place: fm6_sync must never copy an unsanitized byte */
    memcpy(fm6_patch[tr], s, FP_SIZE + 1u);
    RING_PUBLISH();
    fm6_pgen[tr]++;
}

/* SLOT value s < FM6_NFACTORY: that factory patch into track tr */
static void fm6_load_slot(uint32_t tr, uint32_t s)
{
    uint8_t v[FP_SIZE + 1u];
    fm6_unpack(FM6_FACTORY[s % FM6_NFACTORY], v);
    fm6_set_patch(tr, v);
    fm6_slot[tr % NTRK] = (uint8_t)(s % FM6_NFACTORY);
}

/* the SLOT patch p shows: the factory patch it equals (F n; looked for from SLOT s on), else OWN */
static int32_t fm6_match(const uint8_t *p, int32_t s)
{
    uint8_t v[FP_SIZE + 1u];
    uint32_t k, f = s >= 0 && s < (int32_t)FM6_NFACTORY ? (uint32_t)s : 0u;
    for (k = 0; k < FM6_NFACTORY; k++) {
        fm6_unpack(FM6_FACTORY[(f + k) % FM6_NFACTORY], v);
        if (!memcmp(v, p, FP_SIZE))
            return (int32_t)((f + k) % FM6_NFACTORY);
    }
    return FM6_OWN;
}
/* track tr's patch was just put in as its own (a project, a user preset, the editor, a conversion): an FM6 track's
 * SLOT shows the factory patch it equals unchanged (F n; the SLOT it had first), else OWN; nothing reloads over it
 * (fm6_poll) */
static void fm6_adopt(uint32_t tr)
{
    track_t *t = &trk[tr % NTRK];
    int32_t s = t->p[P_E7];
    tr %= NTRK;
    fm6_own_ok &= (uint8_t)~(1u << tr);
    if (t->eng_req == ENGI_FM6)
        t->p[P_E7] = (int16_t)(s = fm6_match(fm6_patch[tr], s));
    fm6_slot[tr] = (uint8_t)s;
}

/* a factory sound load put a SLOT value in (a preset, an engine change): its patch (OWN: the patch stays) */
static void fm6_track_loaded(const track_t *t)
{
    uint32_t tr = (uint32_t)(t - trk);
    if (tr < NTRK && t->eng_req == ENGI_FM6) {
        fm6_own_ok &= (uint8_t)~(1u << tr);
        if (t->p[P_E7] >= 0 && t->p[P_E7] < (int32_t)FM6_NFACTORY)
            fm6_load_slot(tr, (uint32_t)t->p[P_E7]);
        else
            fm6_adopt(tr);
    }
}

/* power-on: every track the init voice (what a project stores for the tracks that never played FM6) */
static void fm6_init(void)
{
    uint8_t v[FP_SIZE + 1u];
    uint32_t tr;
    fm6_unpack(FM6_INIT, v);
    for (tr = 0; tr < NTRK; tr++) {
        fm6_set_patch(tr, v);
        fm6_slot[tr] = 0xFFu;
    }
}

/* main loop: SLOT turned (a knob, the editor, MIDI, motion) -> that factory patch; from OWN the own patch is kept
 * aside (fm6_own), and turning back to OWN brings it back */
static void fm6_poll(void)
{
    uint32_t tr, bit;
    for (tr = 0; tr < NTRK; tr++) {
        int32_t s = trk[tr].p[P_E7];
        if (trk[tr].eng_req != ENGI_FM6 || s == fm6_slot[tr])
            continue;
        bit = 1u << tr;
        if (s < 0 || s >= (int32_t)FM6_NFACTORY) {           /* OWN */
            if (fm6_own_ok & bit) {
                uint8_t v[FP_SIZE + 1u];
                fm6_unpack(fm6_own[tr], v);
                fm6_set_patch(tr, v);
            }
            fm6_own_ok &= (uint8_t)~bit;
            fm6_slot[tr] = FM6_OWN;
            if (s != FM6_OWN)
                trk[tr].p[P_E7] = FM6_OWN;
            continue;
        }
        if (fm6_slot[tr] == FM6_OWN && !(fm6_own_ok & bit)) {
            fm6_pack(fm6_patch[tr], fm6_own[tr]);
            fm6_own_ok |= (uint8_t)bit;
        }
        fm6_load_slot(tr, (uint32_t)s);
    }
}

/* -------------------------------------------------------------- macros --- */
/* RATIO, MOD ENV and VEL MOD (e: P_E0..) on the modulators of patch p in algorithm alg (0..31), as they play: the
 * audio ISR's copy (fm6_sync) and (FM6 LAB) the editor's graph (ui_fm6.c) */
static void fm6_macro(uint8_t *p, uint32_t alg, const int16_t *e)
{
    uint32_t car = fm6_carriers(alg), k, j;
    int32_t meg = clamp(e[4], -64, 63) * 40 / 64;           /* MEG: rate steps (+ slower) */
    for (k = 0; k < 6u; k++) {
        uint8_t *op = p + k * FP_OP;
        if ((car >> k) & 1u)
            continue;
        if (!op[FP_MODE])
            op[FP_FC] = (uint8_t)clamp(op[FP_FC] + e[3], 0, 31);
        for (j = 0; j < 4u; j++)
            op[FP_R1 + j] = (uint8_t)clamp(op[FP_R1 + j] - meg, 0, 99);
        op[FP_KVS] = (uint8_t)clamp(op[FP_KVS] + e[5], 0, 7);
    }
}

/* the patch through E0..E6 (audio ISR; cheap when nothing changed) */
static void fm6_sync(const track_t *t)
{
    uint32_t tr = (uint32_t)(t - trk), k, car, n = 0;
    const int16_t *e = &t->p[P_E0];
    int32_t step;
    if (tr >= NTRK)
        return;
    if (fm6_eff[tr].ok && fm6_eff[tr].gen == fm6_pgen[tr]) {
        for (k = 0; k < 7u && fm6_eff[tr].e[k] == e[k]; k++)
            ;
        if (k == 7u)
            return;
    }
    fm6_eff[tr].gen = fm6_pgen[tr];
    memcpy(fm6_eff[tr].p, fm6_patch[tr], FP_SIZE);
    for (k = 0; k < 7u; k++)
        fm6_eff[tr].e[k] = e[k];
    {
        uint8_t *p = fm6_eff[tr].p;
        uint32_t alg = e[0] >= 1 && e[0] <= 32 ? (uint32_t)e[0] - 1u : p[FP_ALG] & 31u;
        fm6_eff[tr].alg = (uint8_t)alg;
        fm6_eff[tr].fb = (uint8_t)clamp(p[FP_FB] + e[1], 0, 7);
        car = fm6_carriers(alg);
        step = clamp(e[6], 0, 127) * (12 * 13981) / 127;    /* DTUN: up to 12 cents a step (1 cent = 13981) */
        for (k = 0; k < 6u; k++) {                           /* OP1 first: carriers in their order */
            uint32_t ki = 5u - k;
            fm6_eff[tr].dt[ki] = 0;
            if ((car >> ki) & 1u) {                          /* carriers 0, +1, -1, +2, -2, +3 steps apart */
                static const int8_t SPREAD[6] = {0, 1, -1, 2, -2, 3};
                fm6_eff[tr].dt[ki] = SPREAD[n++] * step;
            }
        }
        fm6_macro(p, alg, e);
        fm6_lfo_reset(&fm6_lfo[tr], p);
    }
    fm6_eff[tr].ok = 1;
}

/* --------------------------------------------------------------- voice --- */
static fm6_note_t *fm6_note_of(track_t *t, voice_t *v)
{
    uint32_t i;
    if (t < &trk[0] || t >= &trk[NPART])
        return 0;
    i = (uint32_t)(v - t->v);
    return i < FM6_POLY ? &((fm6_note_t *)eng_part(t))[i] : 0;   /* (Skiff: the part's shared area, engines.c) */
}

static void fm6_note_on(track_t *t, voice_t *v)
{
    fm6_note_t *n = fm6_note_of(t, v);
    uint32_t tr = (uint32_t)(t - trk);
    const uint8_t *p;
    if (!n)
        return;
    fm6_sync(t);
    p = fm6_eff[tr].p;
    fm6_note_init(n, p, (int32_t)v->note + p[FP_TRNSP] - 24, v->mvel ? v->mvel : v->vel,
                  (!v->env && !v->env_out) || !n->live);    /* from silence: phases and gains from 0 */
    fm6_lfo_key(&fm6_lfo[tr]);
}

static void fm6_block(track_t *t)       /* once a block and part: the macros, the LFO */
{
    uint32_t tr = (uint32_t)(t - trk);
    if (tr >= NTRK)
        return;
    fm6_sync(t);
    fm6_lfo_v[tr] = fm6_lfo_sample(&fm6_lfo[tr]);
    fm6_lfo_d[tr] = fm6_lfo_delay(&fm6_lfo[tr]);
}

/* every carrier has gone silent for good: the voice ends (voice.c, engine_t.done) */
static int fm6_done(track_t *t, voice_t *v)
{
    fm6_note_t *n = fm6_note_of(t, v);
    uint32_t tr = (uint32_t)(t - trk);
    if (!n || !n->live)
        return 1;
    if (!fm6_note_done(n, fm6_eff[tr].p, fm6_eff[tr].alg))
        return 0;
    n->live = 0;
    return 1;
}

static void fm6_render(track_t *t, voice_t *v, int32_t *out, uint32_t len, const vmod_t *m)
{
    fm6_note_t *n = fm6_note_of(t, v);
    uint32_t tr = (uint32_t)(t - trk), i;
    const uint8_t *p;
    int32_t bus[FM6_N], lf, fb, lvl, k, a, da;
    if (!n || !n->live || len != FM6_N)
        return;
    p = fm6_eff[tr].p;
    if (!v->gate && n->down)
        fm6_note_key(n, p, 0);
    lf = fm6_note_logfreq(m->pitch16 + (p[FP_TRNSP] - 24) * 16, m->fine);
    fb = clamp(fm6_eff[tr].fb + ((m->shape - (64 << 8)) >> 11), 0, 7);
    lvl = clamp(t->p[P_E2], -64, 63) * 24 + clamp(m->cutoff >> 8, -150, 150) * 16;   /* microsteps */
    if (!fm6_note_compute(n, p, bus, fm6_lfo_v[tr], fm6_lfo_d[tr], lf, fm6_eff[tr].alg, fb, fm6_eff[tr].dt, lvl << 16))
        return;                                         /* every carrier below the threshold */
    /* the voice's amplitude ramp x VOICE_FS (one carrier at full: VOICE_FS / 2), six voices in unison ~ one */
    k = t->p[P_VOICE] == V_UNISON ? VOICE_FS * 2 / 5 : VOICE_FS;
    a = mulq15(m->amp0, k);
    da = (mulq15(m->amp1, k) - a) >> FM6_LG_N;
    for (i = 0; i < FM6_N; i++) {
        int32_t x = bus[i] - n->dc;
        n->dc += x >> 10;                                 /* a one-pole high-pass, ~7 Hz: FM puts sidebands at 0 Hz */
        a += da;
        out[i] += (int32_t)(((int64_t)x * a) >> 26);
    }
}

/* --------------------------------------------------------- the engine --- */
static const char *const N_FM6_ALG[] = {"PAT", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13",
                                        "14", "15", "16", "17", "18", "19", "20", "21", "22", "23", "24", "25",
                                        "26", "27", "28", "29", "30", "31", "32", 0};
static const char *const N_FM6_PATCH[] = {"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "OWN", 0};
_Static_assert(NELEM(N_FM6_PATCH) == FM6_OWN + 2u, "a SLOT name per value");

/* {ALG, FB, MLVL, MRAT, MEG, VMOD, DTUN, SLOT}: the factory patch F1..F8 as it is, DTUN on the pad */
static const preset_t FM6_PRESETS[] = {
    {"TINE EP", {0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 127, 0}, 0, 0, FX(0, 45, 25, 35), TAG(PT_KEYS)},
    {"GLASS BELL", {0, 0, 0, 0, 0, 0, 0, 1}, {0, 0, 127, 0}, 0, 0, FX(0, 10, 30, 70), TAG(PT_KEYS)},
    {"FM BASS", {0, 0, 0, 0, 0, 0, 0, 2}, {0, 0, 127, 0}, 0, 1, FX(0, 0, 10, 10), TAG(PT_BASS)},
    {"FM BRASS", {0, 0, 0, 0, 0, 0, 0, 3}, {0, 0, 127, 0}, 0, 0, FX(0, 25, 20, 40), TAG(PT_LEAD)},
    {"PAD", {0, 0, 0, 0, 0, 0, 30, 4}, {0, 0, 127, 0}, 0, 0, FX(0, 60, 30, 70), TAG(PT_PAD)},
    {"FM MARIMBA", {0, 0, 0, 0, 0, 0, 0, 5}, {0, 0, 127, 0}, 0, 0, FX(0, 0, 25, 40), TAG(PT_KEYS)},
    {"FM ORGAN", {0, 0, 0, 0, 0, 0, 0, 6}, {0, 0, 127, 0}, 0, 0, FX(10, 40, 0, 30), TAG(PT_KEYS)},
    {"NYLON PICK", {0, 0, 0, 0, 0, 0, 0, 7}, {0, 0, 127, 0}, 0, 0, FX(0, 20, 35, 30), TAG(PT_PLUCK)},
    /* (Skiff) for the themes: CRYPT (dungeon synth), NEON (synthwave), QUARK (particles) */
    {"GRAVE BELL", {0, 0, 6, 0, 20, 0, 30, 1}, {0, 0, 127, 0}, 0, 0, FX(0, 10, 15, 100), TAG(PT_KEYS)},
    {"GLASS KEYS", {0, 0, 8, 0, 0, 0, 24, 0}, {0, 0, 127, 0}, 0, 0, FX(0, 75, 30, 45), TAG(PT_KEYS)},
    {"ISOTOPE", {0, 0, 14, 2, -10, 0, 50, 1}, {0, 0, 127, 0}, 0, 0, FX(0, 20, 50, 70), TAG(PT_KEYS)},
    {"DARK MATTER", {0, 4, 16, 0, 10, 0, 0, 2}, {0, 0, 127, 0}, 0, 1, FX(8, 0, 0, 10), TAG(PT_BASS)},
    /* (Skiff phase 9) more for every engine: basses, kits */
    {"PICK BASS", {0, 2, 12, 0, 16, 2, 0, 7}, {0, 0, 127, 0}, 0, 1, FX(0, 0, 5, 10), TAG(PT_BASS)},
    {"GROWL BASS", {0, 5, 8, 0, 20, 0, 8, 3}, {0, 0, 127, 0}, 20, 1, FX(30, 0, 5, 5), MORE({P_LD_FLT, 36}, {P_LRATE, 80}, {P_DTYPE, 2}), TAG(PT_BASS)},
    /* Sillage: variations of the FM6 patches with different ratios, modulation and touch response. */
    {"FM ROUND", {0, -2, -29, 0, 8, 1, 0, 2}, {0, 0, 127, 0}, 0, 1, FX(0, 0, 0, 2), TAG(PT_BASS)},
    {"FM RUBBER", {0, 2, 19, -4, 36, 4, 0, 2}, {0, 0, 127, 0}, 0, 1, FX(0, 2, 0, 5), TAG(PT_BASS)},
    {"FM STEEL BS", {0, 3, 28, -8, 15, 5, 5, 7}, {0, 0, 127, 0}, 0, 1, FX(8, 0, 0, 9), TAG(PT_BASS)},
    {"FM HOLLOW BS", {0, 2, -9, -9, 22, 1, 6, 3}, {0, 0, 127, 0}, 0, 1, FX(3, 0, 0, 7), TAG(PT_BASS)},
    {"GLASS LEAD", {0, 1, -9, -8, 25, 3, 5, 1}, {0, 0, 127, 0}, 0, 1, FX(0, 7, 20, 29), TAG(PT_LEAD)},
    {"FM CUT LEAD", {0, 4, 23, 4, 27, 3, 3, 3}, {0, 0, 127, 0}, 0, 1, FX(7, 6, 12, 18), TAG(PT_LEAD)},
    {"FM FLUTE", {0, -3, -38, -3, -7, 1, 4, 4}, {0, 0, 127, 0}, 0, 1, FX(0, 6, 15, 39),
        MORE({P_LRATE, 76}, {P_LFADE, 84}, {P_LD_PIT, 5}), TAG(PT_LEAD)},
    {"FM PLASMA", {0, 4, 29, 8, 12, 4, 14, 3}, {0, 0, 127, 0}, 0, 1, FX(9, 13, 32, 26),
        MORE({P_GLIDE, 28}), TAG(PT_LEAD)},
    {"GLASS HAZE", {0, -2, -15, 5, 14, 1, 42, 4}, {0, 0, 127, 0}, 0, 0, FX(0, 40, 0, 58),
        MORE({P_LRATE, 30}, {P_LD_FLT, 15}, {P_SPRD, 54}), TAG(PT_PAD)},
    /* Sillage: electric keys, organs and struck keys. */
    {"VELVET EP", {0, -2, -24, 0, -9, 3, 13, 0}, {0, 0, 127, 0}, 0, 0, FX(0, 24, 0, 22), TAG(PT_KEYS)},
    {"TINE KEYS", {0, 1, 12, 0, 20, 4, 4, 0}, {0, 0, 127, 0}, 0, 0, FX(0, 12, 0, 21), TAG(PT_KEYS)},
    {"STAGE EP", {0, 3, 21, 1, 14, 6, 0, 0}, {0, 0, 127, 0}, 0, 0, FX(5, 4, 0, 12), TAG(PT_KEYS)},
    {"DUSK EP", {0, -1, -17, -5, 4, 2, 17, 0}, {0, 0, 127, 0}, 0, 0, FX(0, 15, 6, 32),
        MORE({P_LRATE, 44}, {P_LD_AMP, 20}), TAG(PT_KEYS)},
    {"REED ORGAN", {0, 2, 12, -4, -1, 0, 2, 6}, {0, 0, 127, 0}, 0, 0, FX(5, 21, 0, 18), TAG(PT_KEYS)},
    {"HOLLOW ORGAN", {0, -4, -24, 3, 0, 0, 6, 6}, {0, 0, 127, 0}, 0, 0, FX(0, 26, 0, 23), TAG(PT_KEYS)},
    {"BRIGHT REED", {0, 2, 18, 3, 6, 4, 8, 6}, {0, 0, 127, 0}, 0, 0, FX(3, 6, 0, 17), TAG(PT_KEYS)},
    {"GLASS TINES", {0, -1, -16, -6, -8, 3, 19, 1}, {0, 0, 127, 0}, 0, 0, FX(0, 9, 13, 39), TAG(PT_KEYS)},
    {"HARD MALLET", {0, 4, 24, 6, 10, 6, 1, 5}, {0, 0, 127, 0}, 0, 0, FX(0, 0, 8, 26), TAG(PT_KEYS)},
    /* Sillage: picked, mallet and metallic FM plucks. */
    {"SOFT NYLON", {0, -2, -22, 0, -14, 3, 0, 7}, {0, 0, 127, 0}, 0, 0, FX(0, 2, 0, 12), TAG(PT_PLUCK)},
    {"WIRE PICK", {0, 3, 18, 5, 32, 4, 5, 7}, {0, 0, 127, 0}, 0, 0, FX(0, 0, 11, 18), TAG(PT_PLUCK)},
    {"WOOD CLICK", {0, 2, -20, -8, 26, 6, 0, 5}, {0, 0, 127, 0}, 0, 0, FX(0, 0, 0, 13), TAG(PT_PLUCK)},
    {"FM DROPLET", {0, 0, -26, -3, 17, 2, 5, 5}, {0, 0, 127, 0}, 0, 0, FX(0, 0, 24, 37), TAG(PT_PLUCK)},
    {"GLASS PICK", {0, 1, -6, 12, 22, 3, 8, 7}, {0, 0, 127, 0}, 0, 0, FX(0, 6, 14, 23), TAG(PT_PLUCK)},
    {"BRONZE TICK", {0, 5, 37, -2, 18, 4, 5, 5}, {0, 0, 127, 0}, 0, 0, FX(0, 0, 7, 31), TAG(PT_PLUCK)},
    {"PIXEL PICK", {0, 3, 19, 14, 32, 1, 0, 5}, {0, 0, 127, 0}, 0, 0, FX(12, 0, 12, 9),
        MORE({P_DTYPE, 3}), TAG(PT_PLUCK)},
    {"ECHO MALLET", {0, 1, 6, -4, 2, 2, 16, 5}, {0, 0, 127, 0}, 0, 0, FX(0, 7, 54, 24), TAG(PT_PLUCK)},
    {"BELL SPARK", {0, 3, 15, 9, 22, 4, 15, 7}, {0, 0, 127, 0}, 0, 0, FX(0, 9, 20, 37), TAG(PT_PLUCK)},
    /* Sillage: metallic hits, modulated signals and transition textures. */
    {"METAL FALL", {0, 4, 37, 6, 28, 5, 19, 1}, {0, 0, 127, 0}, 0, 1, FX(0, 4, 0, 50),
        MORE({P_ED_PIT, 52}), TAG(PT_FX)},
    {"GLASS CRASH", {0, 5, 41, 13, 38, 6, 23, 1}, {0, 0, 127, 0}, 0, 0, FX(12, 2, 0, 56), TAG(PT_FX)},
    {"BELL ORBIT", {0, 3, 20, 7, 16, 1, 33, 1}, {0, 0, 127, 0}, 0, 0, FX(0, 19, 34, 49),
        MORE({P_LRATE, 39}, {P_LD_PIT, 14}), TAG(PT_FX)},
    {"DIGI SIREN", {0, 3, 24, 6, 27, 0, 8, 3}, {0, 0, 127, 0}, 0, 1, FX(5, 0, 38, 24),
        MORE({P_LWAVE, 1}, {P_LRATE, 47}, {P_LD_PIT, 47}), TAG(PT_FX)},
    {"DATA GLITCH", {0, 4, 37, -6, 18, 2, 4, 6}, {0, 0, 127, 0}, 0, 1, FX(18, 0, 25, 16),
        MORE({P_LWAVE, 4}, {P_LRATE, 82}, {P_LD_PIT, 38}, {P_DTYPE, 3}), TAG(PT_FX)},
    {"PHASE BEACON", {0, 0, -20, 3, 22, 1, 26, 6}, {0, 0, 127, 0}, 0, 1, FX(0, 28, 26, 25),
        MORE({P_LRATE, 66}, {P_LD_AMP, 48}), TAG(PT_FX)},
    {"FM WORMHOLE", {0, 3, 24, -11, 25, 1, 52, 4}, {0, 0, 127, 0}, 0, 1, FX(0, 29, 19, 62),
        MORE({P_LWAVE, 1}, {P_LRATE, 34}, {P_LD_FLT, 42}, {P_LD_PIT, 17}), TAG(PT_FX)},
    {"ORBIT SWEEP", {0, 2, 29, 11, 40, 1, 62, 4}, {0, 0, 127, 0}, 0, 0, FX(0, 24, 28, 62),
        MORE({P_LRATE, 27}, {P_LD_FLT, 32}, {P_SPRD, 55}), TAG(PT_FX)},
    {"FM THUMP", {0, -1, -14, -2, 43, 6, 0, 2}, {0, 0, 127, 0}, 0, 1, FX(3, 0, 0, 3), TAG(PT_BASS)},
    {"FM REED LEAD", {0, 2, 16, -2, 18, 3, 1, 6}, {0, 0, 127, 0}, 0, 1, FX(0, 6, 14, 28), TAG(PT_LEAD)},
    {"SILVER PICK", {0, 1, 16, 3, -7, 5, 11, 5}, {0, 0, 127, 0}, 0, 0, FX(0, 6, 11, 23), TAG(PT_PLUCK)},
    /* Sillage expansion bank. */
    {"FM SUB WEIGH", {0, 0, 0, 0, 0, 2, 0, 2}, {0, 40, 100, 13}, 0, 1, FX(0, 0, 0, 0), TAG(PT_BASS)},
    {"FM RUBBER PU", {6, 3, -23, -6, 30, 2, 11, 2}, {1, 72, 70, 22}, 0, 1, FX(0, 0, 0, 0), MORE({P_GLIDE, 16}), TAG(PT_BASS)},
    {"FM SYNC BARK", {14, 1, 18, 4, -16, 2, 22, 5}, {0, 25, 48, 17}, 0, 1, FX(0, 0, 0, 0), TAG(PT_BASS)},
    {"FM FRET ATTA", {24, 6, 42, 9, 52, 2, 33, 2}, {0, 55, 18, 15}, 0, 1, FX(0, 0, 0, 0), TAG(PT_BASS)},
    {"FM HOLLOW ST", {3, -3, -40, -3, 10, 2, 44, 0}, {3, 62, 79, 19}, 0, 1, FX(0, 0, 0, 0), TAG(PT_BASS)},
    {"FM LOW MOTOR", {9, 2, 31, 14, -35, 2, 55, 7}, {7, 46, 90, 26}, 0, 1, FX(0, 0, 0, 0), TAG(PT_BASS)},
    {"FM SOLO BRAS", {0, 0, 0, 0, 0, 2, 0, 3}, {5, 63, 100, 31}, 0, 1, FX(0, 0, 0, 0), TAG(PT_LEAD)},
    {"FM GLIDE WIR", {6, 3, -23, -6, 30, 2, 11, 2}, {0, 48, 97, 21}, 0, 1, FX(0, 0, 0, 0), TAG(PT_LEAD)},
    {"FM REED SOLO", {14, 1, 18, 4, -16, 2, 22, 1}, {19, 72, 105, 42}, 0, 1, FX(0, 0, 0, 0), MORE({P_LRATE, 43}, {P_LD_PIT, 4}), TAG(PT_LEAD)},
    {"FM RING SOLO", {24, 6, 42, 9, 52, 2, 33, 3}, {2, 58, 85, 53}, 0, 1, FX(0, 0, 0, 0), TAG(PT_LEAD)},
    {"FM NASAL CUT", {3, -3, -40, -3, 10, 2, 44, 7}, {1, 33, 116, 15}, 0, 1, FX(0, 0, 0, 0), TAG(PT_LEAD)},
    {"FM SLOW BRAS", {0, 0, 0, 0, 0, 2, 0, 4}, {44, 77, 108, 72}, 0, 0, FX(0, 9, 0, 7), TAG(PT_PAD)},
    {"FM PWM VELVE", {6, 3, -23, -6, 30, 2, 11, 1}, {66, 85, 112, 88}, 0, 0, FX(0, 11, 0, 10), TAG(PT_PAD)},
    {"FM DUSK CHOR", {14, 1, 18, 4, -16, 2, 22, 3}, {82, 96, 120, 102}, 0, 0, FX(0, 13, 0, 13), MORE({P_LRATE, 43}, {P_LD_PIT, 4}), TAG(PT_PAD)},
    {"FM FROZEN AI", {24, 6, 42, 9, 52, 2, 33, 4}, {109, 80, 100, 108}, 0, 0, FX(0, 15, 0, 16), TAG(PT_PAD)},
    {"FM DRY TINES", {0, 0, 0, 0, 0, 2, 0, 0}, {0, 76, 36, 38}, 0, 0, FX(0, 0, 0, 0), TAG(PT_KEYS)},
    {"FM STAGE REE", {6, 3, -23, -6, 30, 2, 11, 6}, {5, 86, 72, 49}, 0, 0, FX(0, 0, 0, 0), TAG(PT_KEYS)},
    {"FM SOFT MALL", {14, 1, 18, 4, -16, 2, 22, 5}, {0, 63, 0, 35}, 0, 0, FX(0, 0, 0, 0), TAG(PT_KEYS)},
    {"FM CHURCH KE", {24, 6, 42, 9, 52, 2, 33, 1}, {8, 90, 119, 52}, 0, 0, FX(0, 0, 0, 0), TAG(PT_KEYS)},
    {"FM CLAVI SNA", {3, -3, -40, -3, 10, 2, 44, 7}, {0, 34, 22, 23}, 0, 0, FX(0, 0, 0, 0), TAG(PT_KEYS)},
    {"FM WOOD PICK", {0, 0, 0, 0, 0, 2, 0, 7}, {0, 49, 0, 29}, 0, 0, FX(0, 0, 0, 0), TAG(PT_PLUCK)},
    {"FM METAL PIN", {6, 3, -23, -6, 30, 2, 11, 5}, {0, 82, 0, 63}, 0, 0, FX(0, 0, 0, 0), TAG(PT_PLUCK)},
    {"FM MUTE SNAP", {14, 1, 18, 4, -16, 2, 22, 7}, {0, 29, 0, 12}, 0, 0, FX(0, 0, 0, 0), TAG(PT_PLUCK)},
    {"FM BELL PICK", {24, 6, 42, 9, 52, 2, 33, 1}, {2, 89, 0, 74}, 0, 0, FX(0, 0, 0, 0), TAG(PT_PLUCK)},
    {"FM DUB ALARM", {0, 0, 0, 0, 0, 2, 0, 1}, {4, 74, 86, 42}, 0, 0, FX(0, 0, 0, 9), TAG(PT_FX)},
    {"FM RADIO DRO", {6, 3, -23, -6, 30, 2, 11, 7}, {0, 48, 0, 29}, 0, 0, FX(3, 0, 6, 12), TAG(PT_FX)},
    {"FM METAL CLO", {14, 1, 18, 4, -16, 2, 22, 4}, {68, 100, 90, 100}, 0, 0, FX(6, 0, 12, 15), TAG(PT_FX)},
    {"FM AIR LIFT", {24, 6, 42, 9, 52, 2, 33, 3}, {105, 95, 98, 113}, 0, 0, FX(0, 0, 18, 18), TAG(PT_FX)},
};

static const engine_t ENG_FM6 = {
    .name = "FM6",
    .page_title = {"OPS", "PATCH"},
    .edit = {
        {"ALGO", N_FM6_ALG, 0, 32, 0, F_INT, 0},
        {"FEEDBACK", 0, -7, 7, 0, F_INT, 0},
        {"MOD LVL", 0, -64, 63, 0, F_BIPCT, 0},
        {"RATIO", 0, -16, 16, 0, F_INT, 0},
        {"MOD ENV", 0, -64, 63, 0, F_BIPCT, 0},
        {"VEL MOD", 0, -7, 7, 0, F_INT, 0},
        {"DETUNE", 0, 0, 127, 0, F_PCT, 0},
        {"PATCH", N_FM6_PATCH, 0, FM6_OWN, 0, F_INT, 0},
    },
    .presets = FM6_PRESETS,
    .npresets = NELEM(FM6_PRESETS),
    .note_on = fm6_note_on,
    .render = fm6_render,
    .knob = {P_E2, P_E3, P_E4, P_E7},
    .poly = FM6_POLY,
    .block = fm6_block,
    .ownenv = 1,
    .done = fm6_done,
};
