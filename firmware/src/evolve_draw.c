/* SPDX-License-Identifier: GPL-3.0-only */
static void evolve_draw(void)
{
    /* UI only. The preview uses the very same generator as playback. */
    static uint32_t previous;
    uint32_t sig = song.sel + 1u + (song.playing ? (TSEL->seq_idx / 16u + 1u) * 31u : 0u);
    for (uint32_t k = 0; k < NTRK; k++)
        sig = sig * 33u + evolve_want[k] + evolve_live[k] * 101u + trk[k].eng_req * 10007u +
            evolve_octave[k] * 131u + evolve_len_want[k] * 157u + evolve_len_live[k] * 193u;
    const uint8_t *raw = (const uint8_t *)TSEL->step;
    for (uint32_t j = 0; j < sizeof TSEL->step; j++) sig = sig * 33u + raw[j];
    sig += (uint32_t)TSEL->p[P_SCALE] * 131u + (uint32_t)TSEL->p[P_ROOT] * 157u + (uint32_t)TSEL->p[P_SLEN] * 193u;
    if (!ui.force && previous == sig) return;
    previous = sig;
    /* The three completed canvas strips cover all 240 rows. Sending a blank
     * full-screen fill first causes a visible flash on the physical FM-1. */
    cv_begin(240, 64, T_BG);
    cv_text_in(0, 2, 240, &AF_M, "EVOLVE", T_THEME, T_BG);
    static const char *const octaves[3] = {"0", "+1", "-1"};
    char amount[8], length[8];
    fmt_int(amount, evolve_want[song.sel]);
    str_cpy(amount + str_len(amount), "%", sizeof amount - str_len(amount));
    fmt_int(length, evolve_len_want[song.sel] ? evolve_len_want[song.sel] : 50);
    str_cpy(length + str_len(length), "%", sizeof length - str_len(length));
    const char *labels[3] = {"K1 EVOLVE", "K3 OCTAVE", "K4 LENGTH"};
    const char *values[3] = {amount, octaves[evolve_octave[song.sel]], length};
    for (uint32_t c = 0; c < 3u; c++) {
        cv_text_in((int32_t)c * 80, 25, 80, &AF_S, labels[c], T_DIM, T_BG);
        if (c == 1u) {
            /* Keep every octave visible; gold marks the NEXT additions. */
            for (uint32_t o = 0; o < 3u; o++) {
                int32_t x = 82 + (int32_t)o * 26;
                uint32_t choice = (o + 2u) % 3u;
                int selected = choice == evolve_octave[song.sel];
                uint16_t bg = selected ? T_ACCENT : T_SURF;
                cv_rect(x, 42, 24, 18, bg);
                cv_text_in(x, 45, 24, &AF_S, octaves[choice], selected ? T_INK : T_TEXT, bg);
            }
        } else cv_text_in((int32_t)c * 80, 41, 80, &AF_M, values[c], T_TEXT, T_BG);
    }
    cv_blit(0, 0);
    cv_begin(240, 128, T_BG); /* existing canvas is limited to 138 rows */
    for (uint32_t k = 0; k < NTRK; k++) {
        int allowed = evolve_allowed(&trk[k]);
        int32_t y = (int32_t)k * 16;
        uint16_t bg = k == song.sel ? T_SURF : T_BG;
        uint16_t col = allowed ? T_ACCENT : T_DIM;
        char track[3] = {'T', (char)('1' + k), 0}, value[16];
        cv_rect(3, y, 234, 16, bg);
        cv_text_in(8, y + 2, 28, &AF_S, track, k == song.sel ? T_THEME : T_TEXT, bg);
        cv_rect(40, y + 6, 108, 5, T_DIM);
        if (allowed && evolve_live[k]) cv_rect(40, y + 6, evolve_live[k] * 108 / 100, 5, col);
        if (allowed && (evolve_want[k] != evolve_live[k] || evolve_len_want[k] != evolve_len_live[k])) {
            cv_rect(40 + evolve_want[k] * 107 / 100, y + 3, 2, 11, T_THEME);
            fmt_int(value, evolve_want[k]);
            str_cpy(value + str_len(value), "% WAIT", sizeof value - str_len(value));
        } else if (allowed) {
            fmt_int(value, evolve_live[k]);
            str_cpy(value + str_len(value), "%", sizeof value - str_len(value));
        } else str_cpy(value, "EXCLUDED", sizeof value);
        cv_text_in(156, y + 2, 79, &AF_S, value, col, bg);
    }
    cv_blit(0, 64);
    cv_begin(240, 48, T_BG);
    uint32_t len = (uint32_t)clamp(TSEL->p[P_SLEN], 1, NSTEP);
    uint32_t first = song.playing ? TSEL->seq_idx / 16u * 16u : 0u;
    for (uint32_t j = 0; j < 16u && first + j < len; j++) {
        uint32_t idx = first + j;
        step_t added;
        int original = TSEL->step[idx].n || TSEL->step[idx].hit || TSEL->step[idx].time == ST_TIE;
        int extra = evolve_note(TSEL, idx, evolve_want[song.sel], &added);
        uint16_t color = original ? T_TEXT : extra ? T_THEME : T_DIM;
        cv_rect(8 + (int32_t)j * 14, 1, 10, 8, color);
    }
    cv_text_in(0, 13, 240, &AF_S, "OCT- RESET TRACK", T_TEXT, T_BG);
    cv_text_in(0, 30, 240, &AF_S, "OCT+ RESET ALL", T_THEME, T_BG);
    cv_blit(0, 192);
    ui.force = 0;
}
