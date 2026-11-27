/* SPDX-License-Identifier: GPL-3.0-only */
/* V3: a transient playback overlay. Never write generated notes to steps,
 * the pattern bank or flash. Packed two-bit octaves, no phrase backup/buffer. */
static volatile uint8_t evolve_want[NTRK], evolve_live[NTRK];
static uint8_t evolve_octave[NTRK]; /* UI selection: 0, +1, -1 */
static volatile uint8_t evolve_oct_want[NTRK][16], evolve_oct_live[NTRK][16];
static volatile uint8_t evolve_len_want[NTRK], evolve_len_live[NTRK]; /* 0 = 50% */

static uint32_t evolve_threshold(uint32_t value) { return (value * 64u + 99u) / 100u; }

static void evolve_commit(uint32_t k)
{
    if (!evolve_live[k] && !evolve_want[k] && evolve_len_live[k] == evolve_len_want[k]) return;
    for (uint32_t b = 0; b < 16u; b++) evolve_oct_live[k][b] = evolve_oct_want[k][b];
    evolve_len_live[k] = evolve_len_want[k];
    evolve_live[k] = evolve_want[k];
}

static void evolve_clear(void)
{
    for (uint32_t k = 0; k < NTRK; k++) {
        evolve_want[k] = evolve_live[k] = evolve_octave[k] = 0;
        evolve_len_want[k] = evolve_len_live[k] = 0;
        /* Octave bytes outside the current threshold are never read. */
    }
}

static int evolve_allowed(const track_t *t)
{
    const engine_t *e = ENGINES[eng_idx(t->eng_req)];
    return !drum_engine(eng_idx(t->eng_req)) && !(e->keys && e->keys(t, 0) >= 0);
}

static void evolve_request(uint32_t k, uint32_t value)
{
    if (k >= NTRK) return;
    value = value > 100u ? 100u : value;
    for (uint32_t rank = evolve_threshold(evolve_want[k]); rank < evolve_threshold(value); rank++) {
        uint32_t b = rank >> 2, shift = (rank & 3u) * 2u;
        evolve_oct_want[k][b] = (uint8_t)((evolve_oct_want[k][b] & ~(3u << shift)) |
                                        (uint32_t)evolve_octave[k] << shift);
    }
    /* Publish only after the octave assignment of new candidates is complete. */
    evolve_want[k] = (uint8_t)value;
    if (!song.playing) evolve_commit(k);
}

static void evolve_length(uint32_t k, int32_t steps)
{
    uint32_t old = evolve_len_want[k] ? evolve_len_want[k] : 50u;
    evolve_len_want[k] = (uint8_t)clamp((int32_t)old + steps, 5, 100);
    if (!song.playing) evolve_commit(k);
}

/* Candidate selection is a fixed permutation of the 64 positions. Increasing
 * the threshold only adds candidates: neither notes nor positions reshuffle.
 * Gaps inside ties/slides and explicitly conditioned rests stay untouched. */
static int evolve_note_at(const track_t *t, uint32_t idx, uint32_t amount, step_t *out, int live)
{
    uint32_t len = (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP), k = trk_index(t);
    if (!amount || idx >= len || !evolve_allowed(t)) return 0;
    const step_t *s = &t->step[idx], *prev = &t->step[(idx + len - 1u) % len];
    if (s->n || s->hit || s->time == ST_TIE || s->flags || s->cond || s->probability || s->nudge ||
        prev->time == ST_TIE || (prev->flags & SF_SLIDE) ||
        t->step[(idx + 1u) % len].time == ST_TIE) return 0;
    uint32_t rank = (idx * 37u + k * 11u + 17u) & 63u;
    if (rank >= evolve_threshold(amount)) return 0;
    /* At most one backward scan through the original phrase, only for an
     * added note. An empty phrase remains silent. Preserve existing ties. */
    for (uint32_t d = 1; d <= len; d++) {
        const step_t *src = &t->step[(idx + len - d) % len];
        if (src->time != ST_NOTE || !src->n || src->hit || step_ties(src)) continue;
        uint32_t h = (idx + 1u) * 2654435761u ^ (k + 1u) * 2246822519u;
        static const int8_t interval[8] = {0, 2, 4, 7, 0, -2, 5, 12};
        uint32_t count = src->n > 4u ? 4u : src->n;
        int32_t note = clamp((int32_t)src->note[(h >> 8) % count] + interval[h >> 29], 0, 127);
        note = scale_snap(t, note);
        if (note < 0) note += 12; /* stay in scale even at MIDI's lower bound */
        const volatile uint8_t *oct = live ? evolve_oct_live[k] : evolve_oct_want[k];
        static const int8_t offsets[4] = {0, 12, -12, 0};
        note += offsets[(oct[rank >> 2] >> ((rank & 3u) * 2u)) & 3u];
        while (note > 127) note -= 12;
        while (note < 0) note += 12;
        memset(out, 0, sizeof *out);
        out->time = ST_NOTE; out->n = 1; out->note[0] = (uint8_t)note;
        out->vel = src->vel ? src->vel : 96u;
        return 1;
    }
    return 0;
}

static int evolve_note(const track_t *t, uint32_t idx, uint32_t amount, step_t *out)
{
    return evolve_note_at(t, idx, amount, out, 0);
}
