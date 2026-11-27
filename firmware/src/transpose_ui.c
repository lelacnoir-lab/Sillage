/* SPDX-License-Identifier: GPL-3.0-only */
/* ENV tap retains its page action. A 500 ms hold opens a latched live panel. */
static uint32_t tp_env_start;
static uint8_t tp_env_used;
static int transpose_input(uint32_t *pressed, uint32_t notes)
{
    uint32_t bit = 1u << panel.btn[B_ENV], down = fm1_in.buttons & bit;
    if ((*pressed & bit) && !ui.menu && !ui.confirm && !name_on() && !ui.layer && !ui.uboot) {
        tp_env_start = fm1_ms + 1u;
        tp_env_used = live_transpose.open;
        if (live_transpose.open) { live_transpose.open = 0; ui.force = 1; }
        *pressed &= ~bit;
    }
    if (tp_env_start && down && !tp_env_used && fm1_ms - (tp_env_start - 1u) >= 500u) {
        live_transpose.open = 1;
        tp_env_used = 1;
        ui.force = 1;
    }
    if (tp_env_start && !down) {
        if (!tp_env_used) page_tap(B_ENV);
        tp_env_start = 0;
        tp_env_used = 0;
    }
    if (!live_transpose.open) return 0;
    if (*pressed & (1u << panel.btn[B_HOME])) {
        live_transpose.open = 0;
        *pressed &= ~(1u << panel.btn[B_HOME]);
        go_home();
        return 1;
    }
    if (*pressed & (1u << panel.btn[B_PLAY])) play_press();
    int32_t select = panel_enc(EN_ALGO) + panel_enc(EN_K1);
    if (select) sel_set((uint32_t)clamp((int32_t)song.sel + select, 0, NTRK - 1));
    int32_t include = panel_enc(EN_K2), grid = panel_enc(EN_K3);
    if (include && !drum_engine(eng_idx(TSEL->eng_req))) {
        uint32_t mask = 1u << song.sel;
        live_transpose.mask = (uint8_t)(include > 0 ? live_transpose.mask | mask : live_transpose.mask & ~mask);
    }
    if (grid) live_transpose.grid = (uint8_t)clamp((int32_t)live_transpose.grid + grid, 0, 4);
    (void)panel_enc(EN_SELECT); (void)panel_enc(EN_PRESET); (void)panel_enc(EN_K4);
    for (uint32_t i = 0; i < 27u; i++) if ((notes >> i) & 1u) { transpose_request((int32_t)i - 12); break; }
    return 1;
}
