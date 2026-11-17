/* Squared response of the same shelf/band-pass stages used by the audio EQ. */
static int32_t eq_graph_qdb(const track_t *t, uint32_t bin)
{
    uint32_t w = EQ_FREQ[bin].warp;
    uint32_t x2 = w * w;
    int32_t sum = 0;
    for (uint32_t band = 0; band < 4u; band++) {
        int32_t gain = clamp(t->p[P_EQ_G0 + band], -12, 12);
        if (!gain)
            continue;
        uint32_t f = EQ_FREQ[clamp(t->p[P_EQ_F0 + band], 0, EQ_FREQ_COUNT - 1)].warp;
        uint32_t f2 = f * f;
        uint64_t num, den;
        if (band == 0u || band == 3u) {
            num = band == 0u ? f2 : x2;
            den = (uint64_t)x2 + f2;
        } else {
            int64_t d = (int64_t)x2 - f2;
            num = 2u * (uint64_t)x2 * f2;
            den = (uint64_t)(d * d) + num;
        }
        /* Keep a 14-bit quotient without requiring 64-bit division on the FM-1. */
        while (den > 0x3ffffu) {
            num >>= 1;
            den >>= 1;
        }
        int32_t weight = (int32_t)(((uint32_t)num << 14) / (uint32_t)den);
        int32_t at_center = (int32_t)EQ_POWER_Q14[48 + 4 * gain] - 16384;
        uint32_t power = (uint32_t)(16384 + ((int64_t)weight * at_center >> 14));
        uint32_t lo = 0, hi = 96;
        while (lo < hi) {
            uint32_t mid = (lo + hi + 1u) / 2u;
            if (EQ_POWER_Q14[mid] > power)
                hi = mid - 1u;
            else
                lo = mid;
        }
        if (lo < 96u && power - EQ_POWER_Q14[lo] > EQ_POWER_Q14[lo + 1u] - power)
            lo++;
        sum += (int32_t)lo - 48;
    }
    return sum;
}

static void graph_track_eq(const track_t *t)
{
    enum { X = 27, W = 200, SAMPLES = 67 };
    int32_t top = 8, bottom = (int32_t)cv_h - 17, range = 48;
    int16_t curve[SAMPLES];
    if (bottom - top < 16)
        return;
    for (uint32_t i = 0; i < SAMPLES; i++) {
        int32_t qdb = eq_graph_qdb(t, i * (EQ_FREQ_COUNT - 1u) / (SAMPLES - 1u));
        curve[i] = (int16_t)qdb;
        if (qdb > 96 || qdb < -96)
            range = 192;
        else if (range < 96 && (qdb > 48 || qdb < -48))
            range = 96;
    }
    for (int32_t i = 0; i < 5; i++) {
        int32_t y = top + (bottom - top) * i / 4;
        cv_line(X, y, X + W, y, i == 2 ? T_MID : T_LINE);
    }
    for (int32_t i = 0; i <= 6; i++)
        cv_line(X + W * i / 6, top, X + W * i / 6, bottom, T_LINE);
    char cap[8];
    fmt_int(cap, range / 4);
    cv_text(5, top, &AF_S, cap, T_MID);
    cv_text(7, (top + bottom) / 2 - 4, &AF_S, "0", T_MID);
    fmt_int(cap, -range / 4);
    cv_text(5, bottom - 6, &AF_S, cap, T_MID);
    cv_text(X, bottom + 4, &AF_S, "20 Hz", T_MID);
    cv_text(X + 109, bottom + 4, &AF_S, "1 kHz", T_MID);
    cv_text_r(X + W, bottom + 4, &AF_S, "20 kHz", T_MID, T_SURF);
    int32_t last = top + (range - curve[0]) * (bottom - top) / (2 * range);
    for (uint32_t i = 1; i < SAMPLES; i++) {
        int32_t y = top + (range - curve[i]) * (bottom - top) / (2 * range);
        cv_line(X + W * (i - 1u) / (SAMPLES - 1u), last, X + W * i / (SAMPLES - 1u), y, T_THEME);
        last = y;
    }
    for (uint32_t band = 0; band < 4u; band++) {
        uint32_t bin = (uint32_t)clamp(t->p[P_EQ_F0 + band], 0, EQ_FREQ_COUNT - 1);
        int32_t x = X + W * bin / (EQ_FREQ_COUNT - 1u);
        int32_t y = top + (range - eq_graph_qdb(t, bin)) * (bottom - top) / (2 * range);
        char number[2] = {(char)('1' + band), 0};
        cv_rrect(x - 3, y - 3, 7, 7, 3, T_ACCENT, T_SURF);
        cv_text(x - 3, y > top + 10 ? y - 13 : y + 5, &AF_S, number, T_TEXT);
    }
}
