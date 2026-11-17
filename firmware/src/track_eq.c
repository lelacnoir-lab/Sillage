/* SPDX-License-Identifier: GPL-3.0-only */
/* Four fixed-shape, variable-frequency filters. Each stage adds a scaled low-pass,
 * band-pass or high-pass to the direct signal. A flat band does no sample work. */
typedef struct {
    int64_t z1, z2;
} eq_state_t;
static eq_state_t eq_state[NTRK][4][4] __attribute__((section(".pool")));
static uint8_t eq_live[NTRK];
#define EQ_LIMIT 1048575

static void eq_reset(uint32_t track)
{
    memset(eq_state[track], 0, sizeof eq_state[track]);
    eq_live[track] = 0;
}

static void eq_channel(eq_state_t *state, const track_t *t, int32_t *buf, uint32_t n)
{
    for (uint32_t band = 0; band < 4u; band++) {
        int32_t gain = clamp(t->p[P_EQ_G0 + band], -12, 12);
        if (!gain) {
            state[band].z1 = state[band].z2 = 0;
            continue;
        }
        const eq_freq_t *f = &EQ_FREQ[clamp(t->p[P_EQ_F0 + band], 0, EQ_FREQ_COUNT - 1)];
        int32_t delta = EQ_DELTA[gain + 12];
        int64_t z1 = state[band].z1, z2 = state[band].z2;
        if (band == 0u || band == 3u) {
            int32_t a1 = 2 * f->shelf - (1 << 28);
            for (uint32_t i = 0; i < n; i++) {
                int32_t x = buf[i];
                int64_t in = (int64_t)f->shelf * x;
                int32_t low = (int32_t)((in + z1 + (1 << 27)) >> 28);
                int32_t shaped = band == 0u ? low : x - low;
                z1 = in - (int64_t)a1 * low;
                buf[i] = clamp(x + (int32_t)(((int64_t)delta * shaped + 8192) >> 14), -EQ_LIMIT, EQ_LIMIT);
            }
        } else {
            for (uint32_t i = 0; i < n; i++) {
                int32_t x = buf[i];
                int64_t in = (int64_t)f->band * x;
                int32_t shaped = (int32_t)((in + z1 + (1 << 27)) >> 28);
                z1 = z2 - (int64_t)f->a1 * shaped;
                z2 = -in - (int64_t)f->a2 * shaped;
                buf[i] = clamp(x + (int32_t)(((int64_t)delta * shaped + 8192) >> 14), -EQ_LIMIT, EQ_LIMIT);
            }
        }
        state[band].z1 = z1;
        state[band].z2 = z2;
    }
}

static void track_eq(track_t *t, int32_t *dry, int32_t *send, uint32_t n)
{
    uint32_t k = (uint32_t)(t - trk);
    int32_t *streams[4] = {dry, t->spread_on || t->lpan_on ? SPREAD_BUF(t) : 0,
                         t->lpan_on ? LPAN_BUF(t) : 0, send};
    for (uint32_t i = 0; i < 4u; i++) {
        if (streams[i]) {
            eq_channel(eq_state[k][i], t, streams[i], n);
            eq_live[k] |= (uint8_t)(1u << i);
        } else if ((eq_live[k] >> i) & 1u) {
            memset(eq_state[k][i], 0, sizeof eq_state[k][i]);
            eq_live[k] &= (uint8_t)~(1u << i);
        }
    }
}
