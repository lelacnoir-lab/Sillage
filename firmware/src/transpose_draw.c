/* SPDX-License-Identifier: GPL-3.0-only */
static void transpose_draw(void)
{
    static uint32_t previous;
    static const char *const quant[5] = {"NOW", "1/8", "1/4", "1/2", "BAR"};
    uint32_t sig = (uint32_t)(live_transpose.active + 12) | (uint32_t)(live_transpose.target + 12) << 5 |
        (uint32_t)live_transpose.pending << 10 | (uint32_t)live_transpose.mask << 11 |
        (uint32_t)live_transpose.grid << 19 | song.sel << 22;
    if (!ui.force && sig == previous) return;
    previous = sig;
    lcd_fill(0, 0, 240, 240, T_BG);
    cv_begin(240, 64, T_BG);
    cv_text_in(0, 2, 240, &AF_S, "TRANSPOSE", T_THEME, T_BG);
    const char *labels[4] = {"CHOOSE", "GROUP", "QUANT", ""};
    char track[4] = {'T', (char)('1' + song.sel), 0};
    int is_drum = drum_engine(eng_idx(TSEL->eng_req));
    const char *values[4] = {track, is_drum ? "DRUM" : (live_transpose.mask >> song.sel) & 1u ? "ON" : "OFF",
                            quant[live_transpose.grid % 5u], ""};
    for (uint32_t i = 0; i < 4; i++) {
        cv_rect((int32_t)i * 60, 20, 58, 44, T_SURF);
        cv_text_in((int32_t)i * 60, 22, 58, &AF_S, labels[i], T_DIM, T_SURF);
        if (!i) cv_text_in(0, 32, 58, &AF_S, "TRACK", T_DIM, T_SURF);
        cv_text_in((int32_t)i * 60, 46, 58, &AF_M, values[i], T_TEXT, T_SURF);
    }
    cv_blit(0, 0);
    cv_begin(240, 138, T_BG);
    for (uint32_t i = 0; i < NTRK; i++) {
        int32_t x = 8 + (int32_t)(i % 4u) * 58, y = 6 + (int32_t)(i / 4u) * 26;
        int on = ((live_transpose.mask >> i) & 1u) && !drum_engine(eng_idx(trk[i].eng_req));
        char name[3] = {'T', (char)('1' + i), 0};
        cv_rect(x, y, 50, 23, on ? T_ACCENT : T_SURF);
        cv_text_in(x, y + 5, 50, &AF_S, name, on ? T_INK : T_DIM, on ? T_ACCENT : T_SURF);
    }
    char value[16]; fmt_int(value, live_transpose.active);
    cv_text_in(0, 69, 240, &AF_M, value, T_THEME, T_BG);
    if (live_transpose.pending) {
        char queued[24] = "QUEUED "; fmt_int(queued + 7, live_transpose.target);
        cv_text_in(0, 97, 240, &AF_S, queued, T_TEXT, T_BG);
    } else cv_text_in(0, 97, 240, &AF_S, "KEYBOARD: F4 = ZERO", T_DIM, T_BG);
    cv_blit(0, 72);
    cv_begin(240, 24, T_BG);
    cv_text_in(0, 4, 240, &AF_S, "ENV / HOME: EXIT", T_DIM, T_BG);
    cv_blit(0, 216);
    ui.force = 0;
}
