/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Engine table (order = PRESETS browsing order and the engine numbers of the editor protocol), the
 * the parts' sounds at power-on. */
/* Skiff: the parts' shared engine memory (eng_mem, after the engines): each engine's per-part state lives in its
 * part's area, never in arrays of its own for all the parts */
static void *eng_part(const track_t *t);
#include "dsp.c"
#include "eng_analog.c"
#include "eng_phase.c"
#include "eng_sample.c"
#include "eng_formant.c"
#include "eng_trio.c"
#include "eng_grain.c"
#include "eng_drum.c"           /* DRUM: the 8-lane kit (drum_voice.c) */
#include "eng_drumx.c"          /* (Skiff) DRUM-X: JIANT FM's drum engine (drumx_voice.c, GPL-3.0-only) */
#include "eng_fm6.c"            /* FM6: 6-operator FM, msfa ported (fm6_core.c, Apache-2.0) */
#include "fm4_convert.c"        /* DIGITAL's tables, and its sounds -> FM6 */
#if FELUCCA_FM4
#include "eng_digital.c"        /* DIGITAL: four-operator FM (retired; FELUCCA_FM4=1 builds it) */
#endif
#if FELUCCA_SLICE
#include "eng_slice.c"
#endif

/* A track runs one engine at a time. Only the audio renderer touches this
 * shared state. A switch clears it after fading out the old engine. */
typedef union {
    gr_part_t grain;
    drum_lane_t drum[DV_NLANE];
    drumx_lane_t drumx[DV_NLANE];
    fm6_note_t fm6[FM6_POLY];
#if FELUCCA_SLICE
    int16_t slice[SLC_NV][SLC_RB];   /* SLICE's reverse windows */
#endif
} eng_mem_t;
static eng_mem_t eng_mem[NTRK] __attribute__((section(".pool")));
static void *eng_part(const track_t *t)
{
    uint32_t k = (uint32_t)(t - trk) % NPART;
    return &eng_mem[k];
}
static uint32_t eng_size(const track_t *t)            /* its area's bytes (the engine switch zeroes them) */
{
    (void)t;
    return sizeof(eng_mem_t);
}

/* the editor protocol, user presets and projects store these indices: append, never reorder */
static const engine_t *const ENGINES[NENGINES] = {
    &ENG_ANALOG,                 /* 0 */
#if FELUCCA_FM4
    &ENG_DIGITAL,                /* 1 (ENGI_DIGITAL) */
#else
    &ENG_FM4_GONE,               /* 1: reserved (DIGITAL, retired: its sounds convert to FM6, fm4_convert.c) */
#endif
    &ENG_PHASE,                  /* 2 */
    &ENG_FM4_GONE,                   /* 3 (ENGI_LOFI) */
    &ENG_SAMPLE,                 /* 4 */
    &ENG_FORMANT,                /* 5 VOICE (eng_formant.c: "voice" is a sounding note in voice.c) */
    &ENG_TRIO,                   /* 6 */
    &ENG_FM4_GONE,                  /* 7 */
    &ENG_GRAIN,                  /* 8 */
    &ENG_FM4_GONE,               /* 9: PHYS retired; ID retained for migration */
    &ENG_DRUM,                   /* 10 (ENGI_DRUM) */
    &ENG_FM4_GONE,                  /* 11 */
    &ENG_FM6,                    /* 12 (ENGI_FM6) */
#if FELUCCA_SLICE
    &ENG_SLICE,                  /* 13 (ENGI_SLICE) */
#else
    &ENG_FM4_GONE,               /* 13: reserved (FELUCCA_SLICE=0 builds without SLICE: never offered, eng_ok) */
#endif
    &ENG_DRUMX,                  /* 14 (ENGI_DRUMX, Skiff) */
};

#define RETIRED_PRESET 8u   /* SYNTH PLUCK stands in for a retired sound in old data. */
static int eng_retired(uint32_t e)
{
    return e == ENGI_PHYS || e == ENGI_LOFI || e == ENGI_WHEEL || e == ENGI_NOISE;
}
static void retired_values(int16_t *p)
{
    const preset_t *s = &ANALOG_PRESETS[RETIRED_PRESET];
    for (uint32_t i = 0; i < 8u; i++)
        p[P_E0 + i] = s->e[i];
    for (uint32_t i = 0; i < 4u; i++)
        p[P_ATK + i] = s->env[i];
    p[P_ED_FLT] = s->fenv;
}
#if FELUCCA_LEGACY
static void phys_legacy(int16_t *e)
{
    if (e[0] == 2) {
        e[0] = 0;
        e[6] = 64;
    }
}
#endif

/* a track's engine number as an index (the audio paths: a compare, cheaper than % NENGINES; a bad number: 0) */
static inline uint32_t eng_idx(uint32_t e) { return e < NENGINES ? e : 0u; }

/* the sample source a track's sound plays that has no data (SAMPLE's SET, GRAIN's SRC, SLICE's SRC: an empty or
 * invalid user slot, a set or PIANO this build lacks): its name (it plays a sine: eng_sample.c smp_sine), 0 = none;
 * *code a small number for it (1.., per engine and source) so the UI says it once (ui_input.c sample_notice) */
static uint8_t snd_said[NTRK];   /* per track: the code last said, 0 = none (a project load clears them: said again) */
static const char *snd_missing(const track_t *t, uint32_t *code)
{
    const engine_t *e = ENGINES[eng_idx(t->eng_req)];
    uint32_t si = (uint32_t)t->p[P_E0] % SMP_NALL;
#if FELUCCA_SLICE
    if (e == &ENG_SLICE) {
        si = slc_src_of(t->p);
        *code = 64u + si;
        return slc_get(si) ? 0 : N_SLC_SRC[si];
    }
#endif
    if (e != &ENG_SAMPLE && e != &ENG_GRAIN)
        return 0;
    *code = (e == &ENG_GRAIN ? 32u : 1u) + si % 32u;
    return smp_set_missing(si) ? SMP_ALL_NAMES[si] : 0;
}

/* the order the engines are shown in (PRESETS browsing and its ENG knob, the EDIT layer's keys, the editor's list):
 * engine indices, never DIGITAL's reserved 1 (with FELUCCA_FM4 it follows FM6). The indices stay as they are (the
 * stores and the protocol hold them); only this table orders them */
static const uint8_t ENGINE_ORDER[NENG_SHOWN] = {
    0,                           /* ANALOG */
    12,                          /* FM6 */
#if FELUCCA_FM4
    1,                           /* DIGITAL */
#endif
    2, 4, 5, 6, 8,               /* PHASE SAMPLE VOICE TRIO GRAIN */
#if FELUCCA_SLICE
    13,                          /* SLICE */
#endif
    10,                          /* DRUM */
    14,                          /* DRUM-X */
};

/* eng_ok is for stored sounds and the protocol; eng_pickable is for browsing new factory sounds. */
static int eng_ok(uint32_t e) { return e < NENGINES && !eng_retired(e) && (FELUCCA_FM4 || e != ENGI_DIGITAL) && (FELUCCA_SLICE || e != ENGI_SLICE); }
static int eng_pickable(uint32_t e) { return eng_ok(e); }
static uint32_t eng_vis(uint32_t n) { return ENGINE_ORDER[n % NENG_SHOWN]; }
static uint32_t eng_rank(uint32_t e)
{
    uint32_t n;
    if (!eng_ok(e))
        e = ENGI_FM6;                            /* (DIGITAL without FELUCCA_FM4: its sounds play as FM6) */
    for (n = 0; n < NENG_SHOWN && ENGINE_ORDER[n] != e; n++)
        ;
    return n < NENG_SHOWN ? n : 0u;
}
static uint32_t eng_step(uint32_t e, int32_t dir)
{
    if (e >= NENGINES || eng_retired(e))
        return eng_vis(dir > 0 ? 0u : NENG_SHOWN - 1u);
    return eng_vis((eng_rank(e % NENGINES) + (dir > 0 ? 1u : NENG_SHOWN - 1u)) % NENG_SHOWN);
}
/* Every track can load any installed engine. Retired IDs are reserved for old data. */
static int eng_allowed(uint32_t k, uint32_t e)
{
    return k < NTRK && e < NENGINES && !eng_retired(e);
}
static uint32_t eng_step_for(uint32_t k, uint32_t e, int32_t dir)   /* eng_step past what track k may not play */
{
    uint32_t n = 0;
    do
        e = eng_step(e, dir);
    while (!eng_allowed(k, e) && ++n < NENG_SHOWN);
    return e;
}

/* the parts at power-on (engine, preset; the sequencers empty): bass, pad, lead, drums, (phase 6) a sample sound
 * and a second kit: ANALOG ACID, FM6 PAD, ANALOG SAW LEAD, DRUM KIT, PIANO, DRUM KIT */
static const uint8_t TRK_DEF[NPART][2] = {{0, 4}, {ENGI_FM6, 4}, {0, 0}, {ENGI_DRUM, 0}, {ENGI_SAMPLE, 0}, {ENGI_DRUM, 0},
                                       {0, 0}, {0, 0}};
static uint32_t trk_def_engine(uint32_t i) { return TRK_DEF[i % NPART][0]; }
