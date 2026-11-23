/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Felucca quick layers: while a page button is held, the keys and KNOB 1..4
 * are its shortcuts and its map shows over the page. One table (LAYERS) drives the gesture, the keys, the knobs,
 * the LEDs and the overlay:
 *   FX   HOLD  the performance effects (perform.c) and the track mutes while held; KNOB 1..4 its macros
 *   GLO  SET   black keys F#3 G#3 A#3 C#4 D#4 F#4: T1..T6 MUTE (latched; lit = sounding); F3..D4 SOLO
 *              T1..T6 while held (HOLD cells, a corner triangle), E4 UNMUTE ALL, F4 FILL, G4 TAP tempo; KNOB 1..4
 *              the LEVEL of the four tracks shown (ui.c trk_top);
 *              GLO + PLAY: from the top without stopping
 *   SCL  SET   any key: its note name is ROOT; KNOB 1..4 ROOT SCL CHRD VOIC (LY_SCL: the SCL page's first two,
 *              the CHORD page's two; QNT TRN stay on SCL); the LEDs show the root lit and the scale's notes breathing
 *   EDIT SET   the white keys from F3: Sillage's sound families, TOUT, FAVORIS, then INIT.
 *              (the dialog); KNOB 1 ENG, 2 No. (the engine's sounds), 3 FAV. Sound loads as on PRESETS: the steps
 *              stay, SAVE held undoes, the editor gets RELOAD; they apply while playing too
 *   SEQ  SET   ((Skiff U2) on every page; up to U1 only on the SEQ pages that show the pattern, SEQ held elsewhere
 *              opening SONG) TOOLS: the white keys from F3 the sequence tools (ui_tools.c: CLEAR,
 *              REVERSE, SHIFT < >, RANDOM, COOK; on a DRUM track BEAT and the lane's CLEAR REVERSE FILL RANDOM), black
 *              keys 1..8 the lane (DRUM); KNOB 1..4 the PATTERN page's LEN DIV SWING GATE. Each action one undo (SAVE
 *              held), OCT- the track as the layer opened (tl_put_back)
 *   REC  SET   (1.1.5; (Skiff U2) REC is a transport button again, arming on its press: the layer opens locked by
 *              SHIFT + REC only) the white keys from F3: CLEAR (held for the hold time: the selected track's steps and
 *              automation, no dialog: one undo, SAVE held), CLICK (OFF REC ON), COUNT-IN (OFF 1 BAR
 *              2 BARS), CLICK LEVEL (LOW MID HIGH), each press the next value; KNOB 1..3 the same three, (Skiff) KNOB 4
 *              FEEL (OFF ON: REC and TAKES keep a note's time as its step's NUDGE, seq.c feel_of). The settings
 *              are MENU > AUDIO's (ui.c rp_get / rp_put: one store), saved when the layer closes; OCT- puts back the
 *              settings and the track as the layer opened (tl_put_back). Replaces the old REC held on SEQ (Discussion #91:
 *              the CLEAR Tn SEQUENCE? dialog and its "HOLD: CLEAR" hint)
 * The gesture: let go before HOLD (the menu: 0.3 .. 0.6 s) with nothing else touched: a tap, the button's page.
 * Held past HOLD alone: the map (a peek), letting go does nothing. A key, a knob or a button meanwhile: a combo,
 * the map at once, no tap. Keys pressed with the button down are the layer's (seq.c keyboard_block): silent, no
 * MIDI, never recorded; keys held before stay notes. Only one layer at a time: a second layer button is ignored.
 * PLAY and REC work in every layer (GLO held + PLAY: RESTART; GLO's lock: PLAY as always, Felucca #183); SAVE, HOME, SEQ and the other page buttons are swallowed. In SET
 * layers OCT± do not shift the octave: OCT- (on release, not with OCT+) puts back what the layer changed since it
 * opened. A HOLD key's effect lasts until the key is let go, the map with it. No layer in the menu, a dialog,
 * NAME or the UPDATE MODE countdown. After a tap, until the layer has been opened once: "HOLD [GLO] QUICK"
 * (the seen bits are kept with the settings, favorites.c spare byte).
 * The lock ((Skiff U2, the review's G2; it was a double tap, #83, which two quick page taps set off): SHIFT (HOME
 * held) + a layer's button, or (#13) the button held + HOME, opens its map and keeps it open with no button held
 * (ui.lock), over the page as it is:
 * ly_down() counts the lock as the button held, so the keys (seq.c keyboard_block: kb_lock), KNOB 1..4, OCT-, PLAY act
 * exactly as held. A locked FX layer latches (perf_latch_on: an effect key toggles; it was MENU > FX LATCH). A tap of
 * its button closes it (the press hands it back to the button: held on, a peek; let go, closed; never a tap), so does
 * SHIFT + its button, another page button, SAVE, SEQ (each then acting as always), the menu, a dialog, NAME. */
#include "ui_tools.c"                                   /* SEQ TOOLS' actions */
enum { LK_HOLD, LK_SET };
typedef struct {
    uint8_t btn, kind, fam;            /* the button, HOLD / SET, the family whose first page KNOB 1..4 edit */
    const char *head;                  /* (FAM_HOME: the layer's own knobs) the header over the map */
    khint_t foot[3];                   /* the footer's key hints (a third with no word: none) */
} layer_t;
static const layer_t LAYERS[LAYER_N] = {
    {0, 0, 0, "", {{0, 0}, {0, 0}, {0, 0}}},
    {B_FX, LK_HOLD, FAM_HOME, "[FX] HOLD", {{KC_WHITE, "EFFECTS"}, {KC_BLACK, "MUTE"}, {KC_K14, "MACROS"}}},   /* (LET GO: the
                                                     * header's HOLD; (2.0, D 1A) BLACK: GLO's MUTE) */
    {B_GLO, LK_SET, FAM_HOME, "[GLO] SET", {{KC_PLAY, "RESTART"}, {KC_OCTDN, "UNDO"}, {KC_GLO, "DONE"}}},
    {B_SCL, LK_SET, FAM_SCL, "[SEL] SET", {{KC_KEYS, "ROOT"}, {KC_OCTDN, "UNDO"}, {KC_SCL, "DONE"}}},
    {B_EDIT, LK_SET, FAM_HOME, "[EDIT] FAMILLES", {{KC_WHITE, "FAMILLE"}, {KC_OCTDN, "UNDO"}, {KC_EDIT, "DONE"}}},
    {B_SEQ, LK_SET, FAM_SEQ, "[SEQ] TOOLS", {{KC_KEYS, "TOOLS"}, {KC_OCTDN, "UNDO"}, {KC_SEQ, "DONE"}}},
    {B_REC, LK_SET, FAM_HOME, "[REC] SET", {{KC_KEYS, "RECORDING"}, {KC_OCTDN, "UNDO"}, {KC_REC, "DONE"}}},
    {B_ARP, LK_SET, FAM_HOME, "[ARP] PERFORM", {{KC_SELECT, "LAUNCH"}, {KC_PRESETS, "KEYS"}, {KC_OCTDN, "CANCEL"}}},   /* (Skiff;
                                                     * the footer drawn as the B mockup's: draw_layer; BLOCKS' header too:
                                                     * ui_blocks.c) */
};
static const uint8_t LY_KC[LAYER_N] = {0, KC_FX, KC_GLO, KC_SCL, KC_EDIT, KC_SEQ, KC_REC, KC_ARP};
/* SCL's knobs: the key and its chord (cur_page() while the layer edits or draws them: page_over) */
static const page_t LY_SCL = {"SCALE", FAM_SCL, SC_TRACK, GR_SCALE, {P_ROOT, P_SCALE, P_CHRD, P_VOIC}};
/* SEQ TOOLS' knobs: the PATTERN page's (the length the tools work in) */
static const page_t LY_SEQ = {"PATTERN", FAM_SEQ, SC_TRACK, GR_STEPS, {P_SLEN, P_SDIV, P_SSWING, P_SGATE}};
static const page_t *ly_page(uint32_t l) { return l == LAYER_SCL ? &LY_SCL : l == LAYER_SEQ ? &LY_SEQ : 0; }
#define LY_OPEN 2u                     /* ui.ly_t0: the map opened (no tap any more) */
#define LY_COMBO 4u                    /* .. by a combo */
#define LY_DEAD 8u                     /* .. pressed where there is no layer: does nothing */
#define LY_INIT ((uint32_t)PT_N + 1u)  /* EDIT: seven families, TOUT, FAVORIS, then INIT */
typedef char ly_init_fits[LY_INIT < 16u ? 1 : -1];   /* (a white key: F3 .. G5) */
#define layer_seen (favorites.factory[15][31])   /* bit l: layer l opened once (a byte no engine uses) */
static const khint_t FX_LATCH_FOOT[3] = {{KC_WHITE, "ON / OFF"}, {KC_BLACK, "MUTE"}, {KC_OCTDN, "ALL OFF"}};
static const khint_t GLO_LOCK_FOOT[3] = {{KC_OCTDN, "UNDO"}, {KC_GLO, "DONE"}, {0, 0}};   /* (locked: PLAY plays) */
static const khint_t EU_FOOT[3] = {{KC_OCTUP, "APPLY"}, {KC_OCTDN, "CANCEL"}, {0, 0}};   /* ((1.3) SEQ TOOLS' EUCLID open) */

static struct {
    uint8_t l, trk, loaded, oct;       /* SET: the layer and the track of the snapshot; EDIT: a sound loaded since
                                        * it opened (SEQ: a tool used); OCT- / OCT+ pressed in a SET layer (bits) */
    uint8_t cook, preset_list;         /* SEQ: COOK pressed; EDIT: previous track filter for OCT- */
    uint8_t rp_dirty;                  /* REC: a setting changed (settings_save when the layer lets go) */
    uint8_t rec_tracks[NTRK];          /* REC: track settings as the layer opened, for OCT- undo */
    int16_t v[2u * NTRK + 1u];         /* the values when it opened: GLO every track's mute, then level (NTRK + k); SCL
                                        * ROOT..TRN, CHRD VOIC; SEQ LEN DIV SWING GATE; REC the settings (ui_rec_prefs);
                                        * the BPM last (LY_BPM) */
    uint32_t solo;                     /* GLO: the keys held that solo */
    uint32_t fill;                     /* GLO: D4 held, FILL (Skiff: seq.c seq_fill, the FILL / !FILL steps) */
    uint32_t tap[4];                   /* GLO TAP: the last taps (fm1_ms) */
    uint8_t ntap;
    uint8_t ret;                       /* ((2.0, D 2A) the footer's way back: 1 + RETURN, 0 none) */
    uint8_t quiet;                     /* #39: a layer closed after it opened: KNOB 1..4 do nothing until quiet_t + */
    uint32_t quiet_t;                  /* LY_QUIET_MS (fm1_ms) */
    uint32_t reset_all_at;             /* G5 pressed in SEQ TOOLS, 0 while idle */
    uint8_t reset_all_slot, reset_all_track;
    uint32_t clr_t;                    /* (Skiff U2) REC's CLEAR key pressed (fm1_ms | 1; 0: none): it clears when held */
    uint8_t mute[NTRK];                /* ((1.3) EDIT: the DRUM lanes' mutes when it opened (eng_drum.c drum_mute; OCT-) */
} lys;
#define LY_BPM (2u * NTRK)             /* lys.v: the BPM's */
#define LY_QUIET_MS 250u
#define SEQ_ALL_PLACE 15u              /* G5, the last white key */
#define SEQ_ALL_KEY (1u << 26)
#define SEQ_ALL_HOLD_MS 1000u

static int layer_allowed(void) { return !ui.menu && !ui.confirm && !ui.uboot && !name_on(); }
static uint32_t ly_bit(uint32_t l) { return 1u << panel.btn[LAYERS[l].btn]; }
/* a layer's button held opens it, on every page ((Skiff U2) SEQ's too); REC's never (it arms on its press: SHIFT + REC) */
static int ly_avail(uint32_t l) { return l != LAYER_REC; }
static uint32_t ly_down(uint32_t l) { return l && ((fm1_in.buttons & ly_bit(l)) != 0u || ui.lock == l); }   /* (locked: held) */
static uint32_t layer_bits(void)
{
    uint32_t l, m = 0;
    for (l = LAYER_FX; l < LAYER_N; l++)
        m |= ly_avail(l) ? ly_bit(l) : 0u;
    return m;
}
static uint32_t layer_btn(void) { return LAYERS[ui.layer % LAYER_N].btn; }
static const char *const REC_GRID_NAME[] = {"TRACK", "1/2", "1/2 D", "1/2 T", "1/4", "1/4 D", "1/4 T",
                                          "1/8", "1/8 D", "1/8 T", "1/16", "1/16 D", "1/16 T", "1/32"};
_Static_assert(NELEM(REC_GRID_NAME) == NELEM(REC_GRID_TICKS), "recording grid labels");
static const char *layer_head(void) { return ui.layer == LAYER_FX && perf_latch_on ? "[FX] LATCH" : LAYERS[ui.layer % LAYER_N].head; }
static uint32_t layer_open(void) { return ui.ly && (ui.ly_t0 & LY_OPEN) && ly_down(ui.ly) && layer_allowed() ? ui.ly : 0u; }
static int layer_set_open(void)                 /* (FX LATCH: FX is one too, OCT- turns all off) */
{
    return layer_open() && (LAYERS[layer_open()].kind == LK_SET || (layer_open() == LAYER_FX && perf_latch_on));
}
static uint32_t layer_held(void) { return ui.ly && ly_down(ui.ly) && layer_allowed(); }

/* each pass, before the keys: what the ISR (seq.c keyboard_block) gives to a layer. Armed: its button; none
 * armed: any layer button not already down (one pressed meanwhile is armed below; one held over from before
 * is ignored, its keys stay notes); armed but dead (pressed under a dialog, a menu or NAME that has closed
 * since): nothing, its keys stay notes and FX starts no effect with no map shown; FX's effects only with FX */
static void layer_masks(void)
{
    uint32_t m = !layer_allowed() || (ui.ly && (ui.ly_t0 & LY_DEAD)) ? 0u
               : ui.ly ? ly_bit(ui.ly) : layer_bits() & ~fm1_in.buttons;
    kb_mask = m;
    perf_mask = m & ly_bit(LAYER_FX);
    kb_lock = (uint8_t)(m && ui.lock ? 1u | (ui.lock == LAYER_FX ? 2u : 0u) : 0u);   /* (no button held: the ISR's) */
}

/* a layer button pressed (its edge) while none is armed: it is now, a dead one where no layer opens */
static void layer_arm(uint32_t pressed, uint32_t now)
{
    uint32_t l, other = 0;
    if (ui.ly) {                                        /* ((Skiff U4, F7) a second layer's button: ignored, and said) */
        for (l = LAYER_FX; l < LAYER_N; l++)
            if (l != ui.ly && ly_avail(l))
                other |= ly_bit(l);
        if ((pressed & other) && layer_open())
            ui_message("ONE LAYER AT A TIME");
        return;
    }
    for (l = LAYER_FX; l < LAYER_N; l++)
        if ((pressed & ly_bit(l)) && ly_avail(l)) {
            ui.ly = (uint8_t)l;
            ui.ly_t0 = (now & ~15u) | 1u | (layer_allowed() ? 0u : LY_DEAD);
            return;
        }
}

/* the armed layer lets go (its button up, or its lock closed): FX's macros come back once its map has gone ((2.0, D 2A)
 * over RETURN: ui.c mom_poll; (2.0.3, #69) not while a key pressed with FX is held; FX LATCH: they stay while it is
 * locked); one that opened, or a combo: KNOB 1..4 quiet for a while (#39: the
 * knob still turning is not the page's) */
static void layer_let_go(uint32_t quiet)
{
    if (eu.on)                                          /* ((1.3) SEQ TOOLS: an open EUCLID is applied) */
        eu_close(&trk[lys.trk % NTRK], 1);
    if (ui.ly == LAYER_FX)                              /* (a lock let go: its latched effects with it) */
        perf_latched = 0;
    upin.pin = 0;                                       /* ((2.0) SEQ TOOLS', REC's put-back: the ring as ever) */
    upin.pin_next = 0;
    lys.clr_t = 0;
    lys.reset_all_at = 0;
    if (lys.rp_dirty) {                                 /* REC: its settings kept, as MENU does when it closes */
        lys.rp_dirty = 0;
        settings_save();                                /* (deferred while playing) */
    }
    if (quiet) {
        lys.quiet = 1;
        lys.quiet_t = fm1_ms;
    }
    ui.ly_t0 = 0;
    ui.ly = 0;
    ui.lock = 0;
}

/* each pass before the layer's buttons are read: a locked layer closes. Its own button pressed: the lock goes, the
 * button held keeps it open (a peek: let go, it closes, no tap). Another button but PLAY, REC, HOME and OCT- / OCT+
 * (another page button, another layer's, SAVE, SEQ): closed now, and that button acts as always ((Skiff U2) HOME: SHIFT,
 * its tap goes HOME anyway and the lock with it, layer_allowed). (Skiff) PERFORM's KEYS SCENE: SAVE is the layer's
 * (held + a key stores), so it stays open */
static void layer_lock_input(uint32_t pressed)
{
    uint32_t keep = 1u << panel.btn[B_PLAY] | 1u << panel.btn[B_REC] | 1u << panel.btn[B_OCTDN] | 1u << panel.btn[B_OCTUP] |
                    (ui.lock == LAYER_PERF && perf_scenes() ? 1u << panel.btn[B_SAVE] : 0u);
    if (!ui.lock)
        return;
    if (pressed & ly_bit(ui.lock))
        ui.lock = 0;
    else if (pressed & ~keep & ~(1u << panel.btn[B_HOME]))
        layer_let_go(1);
}
static int layer_locked(void) { return ui.lock && ui.layer == ui.lock; }   /* (the map's header: the lock's mark) */

static void layer_opened(uint32_t l);
/* (Skiff U2) SHIFT + a layer's button (pressed: the button's bit): that layer locked open over the page as it is; its
 * own lock: closed. Another layer open or locked closes first */
static void layer_opened(uint32_t l);
static void layer_shift_lock(uint32_t pressed, uint32_t now)
{
    uint32_t l;
    for (l = LAYER_FX; l < LAYER_N && !(pressed & ly_bit(l)); l++)
        ;
    if (l == LAYER_N || !layer_allowed())
        return;
    if (ui.lock == l) {
        layer_let_go(1);
        ui.force = 1;
        return;
    }
    if (ui.ly)
        layer_let_go(1);
    ui.ly = (uint8_t)l;
    ui.ly_t0 = (now & ~15u) | 1u | LY_OPEN;
    ui.lock = (uint8_t)l;
    ui.entry_open = 0;
    ui.force = 1;
    layer_opened(l);
}
static void layer_unlock(void) { if (ui.lock) layer_let_go(1); }   /* (HOME tapped: ui_input) */
/* (#13) a layer's button held, then HOME: locked, as SHIFT + the button (the press is a combo: the map opens this pass,
 * its snapshot kept if it was open). REC never: it is never armed held (ly_avail); a dead one never */
static void layer_home_lock(void)
{
    if (ui.ly && !(ui.ly_t0 & LY_DEAD)) {
        ui.lock = ui.ly;
        ui.entry_open = 0;
        ui.force = 1;
    }
}
/* every layer's button (SHIFT + one locks it), REC's and SEQ's on every page */
static uint32_t layer_bits_all(void)
{
    uint32_t l, m = 0;
    for (l = LAYER_FX; l < LAYER_N; l++)
        m |= ly_bit(l);
    return m;
}

/* the layer opened: once seen, no more hint; SET: what OCT- will put back */
static void layer_opened(uint32_t l)
{
    uint32_t k;
    if (!((layer_seen >> l) & 1u)) {
        layer_seen |= (uint8_t)(1u << l);
        settings_save();                                /* (deferred while playing) */
    }
    lys.l = (uint8_t)l;
    lys.trk = song.sel;
    lys.loaded = 0;
    lys.cook = 0;
    lys.preset_list = ui.preset_lists[song.sel % NTRK];
    if (l == LAYER_SEQ || l == LAYER_REC) {             /* the track as it is now (OCT-): ((2.0) the undo copy of the */
        upin.pin = 0;                                   /* first change, ui.c upin) */
        upin.pin_next = 1;
    }
    for (k = 0; k < 4u; k++)
        if (l == LAYER_SEQ) {
            lys.v[k] = TSEL->p[P_SLEN + k];
        } else if (l != LAYER_GLO) {
            lys.v[k] = TSEL->p[P_ROOT + k];
            lys.v[4u + k] = k < 2u ? TSEL->p[P_CHRD + k] : 0;
        }
    for (k = 0; l == LAYER_GLO && k < NTRK; k++) {
        lys.v[k] = trk[k].p[P_MUTE];
        lys.v[NTRK + k] = trk[k].p[P_LEVEL];
    }
    lys.v[LY_BPM] = song.g[G_BPM];
    if (l == LAYER_REC) {
        lys.v[0] = ui_rec_prefs;
        memcpy(lys.rec_tracks, rec_track_prefs, sizeof lys.rec_tracks);
        rec_grid_page = 0;
    }
    memcpy(lys.mute, drum_mute, sizeof lys.mute);
}

/* the armed button: 0, or the layer whose button was tapped (let go) */
static uint32_t layer_gesture(uint32_t now, uint32_t combo)
{
    uint32_t *t0 = &ui.ly_t0, l = ui.ly;
    if (!l)
        return 0;
    if (!layer_allowed()) {
        *t0 |= LY_DEAD;
        ui.lock = 0;                                    /* (the menu, a dialog, NAME: the lock closes) */
    }
    if (ly_down(l)) {
        int held = now - (*t0 & ~15u) >= (uint32_t)HOLD_MS[settings_hold % 4u] * 1000u * FM1_TICKS_PER_US;
        if (!(*t0 & (LY_OPEN | LY_DEAD)) && (combo || held)) {
            *t0 |= LY_OPEN | (combo ? LY_COMBO : 0u);
            layer_opened(l);
            if ((l == LAYER_PERF || l == LAYER_FX) && held)
                layer_home_lock();
        }
        return 0;
    }
    l = *t0 & (LY_OPEN | LY_DEAD) || combo ? 0u : l;   /* (a knob turned as it was let go: a combo, no tap) */
    layer_let_go(*t0 & LY_OPEN || combo);
    return l;
}

/* #39: KNOB 1..4 belong to no page while a layer lets go: the frame its button is let go (the turns read then
 * were made with it held), while its map still shows (its keys held after it), and LY_QUIET_MS after it closed.
 * Their turns are dropped there (ui_input); before, they edited the page under the layer (an ARP page: ARP on) */
static int layer_knobs_quiet(void)
{
    if (lys.quiet && fm1_ms - lys.quiet_t >= LY_QUIET_MS)
        lys.quiet = 0;
    return layer_allowed() && (ui.ly || ui.layer || lys.quiet);
}

/* the map shows while the button is held open, and after it while its keys are still held */
static void layer_show(void)
{
    uint32_t show = layer_allowed() ? (layer_open() ? layer_open() : kb_layer ? ui.layer : 0u) : 0u;
    if (show != ui.layer) {
        ui.layer = (uint8_t)show;
        ui.force = 1;
    }
}

/* (2.5) the first phrase played stopped (8 notes since the stop: take.c's log, an entry for each start and end) says
 * once how to keep it (ui_take.c's FREE TAKE); bit 0 of layer_seen (no layer's). Not over a message, a dialog, the
 * menu; MENU > HINTS OFF: never */
static void free_hint(void)
{
    if ((layer_seen & 1u) || song.playing || free_n - free_n0 < 16u || ui.msg_t || ui.confirm || ui.menu || ui_hints_off)
        return;
    layer_seen |= 1u;
    settings_save();
#if BOARD_SMK37
    ui_message("SHIFT + BT: FREE TAKE");               /* (the SMK's SAVE: BT) */
#else
    ui_message("SHIFT + SAVE: FREE TAKE");
#endif
    ui.msg_t = 130;                                     /* (as a layer's hint, ~2 s) */
}

/* a tap: the button's page; the hint until its layer has been opened once (unless the tap said something; never
 * over a dialog or the menu) */
static void layer_tap(uint32_t l)
{
    uint8_t m = ui.msg_t;
    (void)page_tap(LAYERS[l].btn);                      /* ((Skiff U2) REC is no layer button: its press arms, ui_input) */
    if (!((layer_seen >> l) & 1u) && ui.msg_t == m && !ui.confirm && !ui.menu && !ui_hints_off) {
        ui_say("HOLD [", KC[LY_KC[l]].label);
        str_cpy(ui.msg + str_len(ui.msg), "]: SHORTCUTS", sizeof ui.msg - str_len(ui.msg));   /* ((Skiff U4) was QUICK) */
        ui.msg_t = 130;                                 /* ~2 s ((Skiff U1) longer than a message, MSG_T) */
    }
}

/* EDIT: a sound load in the layer. The first one takes the undo copy as the track is now (when the layer
 * opened, unless KNOB 2 / 3 had loaded already), so OCT- and SAVE held go back to it */
static uint32_t snd_id(void) { return TSEL->eng_req | (uint32_t)TSEL->preset << 8 | (uint32_t)TSEL->user << 16; }
static void edit_load(uint32_t e, int32_t step)
{
    uint32_t id = snd_id();
    if (!lys.loaded)
        undo.keep = 0;
    if (e < NENGINES) {
        if (e == TSEL->eng_req % NENGINES)
            return;                                     /* its engine already: the sound stays */
        select_engine(e);
        ui.force = 1;
    } else {
        eng_list_step(step);
    }
    lys.loaded |= snd_id() != id;
}

/* White keys 1–7 select musical families; the next two clear or narrow the list. */
static uint32_t edit_list_mode(uint32_t key)
{
    return key < PT_N - 1u ? key + 2u : key - (PT_N - 1u);
}

static void edit_family(uint32_t key)
{
    uint32_t i;
    ui.preset_lists[song.sel % NTRK] = (uint8_t)(edit_list_mode(key) + 1u);
    for (i = 0; i < NPAGES; i++)
        if (PAGES[i].graph == GR_BROWSE) {
            ui.home = 0;
            ui.page = (uint8_t)i;
            page_entered();
            break;
        }
}

/* GLO TAP: from the third tap the tempo of the taps (the last 4); 2 s without one starts over. INT clock only */
static void glo_tap(void)
{
    uint32_t n = lys.ntap, i;
    if (song.g[G_CLOCK]) {
        ui_message("TAP: CLOCK IS EXTERNAL");
        return;
    }
    if (n && fm1_ms - lys.tap[n - 1u] > 2000u)
        n = 0;
    if (n == 4u) {
        for (i = 0; i < 3u; i++)
            lys.tap[i] = lys.tap[i + 1u];
        n = 3;
    }
    lys.tap[n++] = fm1_ms;
    lys.ntap = (uint8_t)n;
    if (n < 3u)                                         /* ((Skiff U4, F7) the taps before the tempo: counted) */
        ui_message(n == 1u ? "TAP 2 MORE" : "TAP 1 MORE");
    if (n >= 3u && lys.tap[n - 1u] != lys.tap[0]) {
        song.g[G_BPM] = (int16_t)clamp((int32_t)(60000u * (n - 1u) / (lys.tap[n - 1u] - lys.tap[0])),
                                       GP[G_BPM].min, GP[G_BPM].max);
        ui.bpm_t = 40;
    }
}

/* SEQ TOOLS: white key place p's action on the selected track (ui_tools.c), "STOP TO EDIT" while a song plays */
static void tools_key(uint32_t p)
{
    const char *m;
    char n[4];
    if (!tl_cell(TSEL, p))
        return;
    if (eu.on) {                                        /* ((1.3) EUCLID open: applied; F4 again does only that) */
        eu_close(TSEL, 1);
        if (p == TL_EUCLID) {
            ui_message("EUCLID APPLIED");
            ui.force = 1;
            return;
        }
    }
    if (p == TL_EUCLID) {                               /* (its message: eu_say; one undo copy for all of it) */
        eu_open(TSEL);
        if (!lys.loaded)
            str_cpy(ui.msg2, "HOLD [SAVE]: UNDO", sizeof ui.msg2);
        lys.loaded = 1;
        ui.force = 1;
        return;
    }
    if (!(m = tl_do(TSEL, p, ui.lane)))
        return;
    if (p == TL_COOK && lys.cook < 99u) {               /* "COOKED 3": the third since the layer opened */
        lys.cook++;
        fmt_int(n, lys.cook);
        ui_say("COOKED ", n);
    } else {
        ui_message(m);
    }
    if (!lys.loaded)                                    /* (the first one: how to take it back) */
        str_cpy(ui.msg2, "HOLD [SAVE]: UNDO", sizeof ui.msg2);
    lys.loaded = 1;
    ui.force = 1;
}

static void seq_clear_column(uint32_t slot);

static void seq_clear_all_start(void)
{
    if (!layer_open() || eu.on)
        return;
    if (transport_busy()) {
        ui_message("STOP TO CLEAR ALL");
        return;
    }
    uint32_t slot = !ui.home && PAGES[ui.page].graph == GR_SONG ? sg.entry : lists.cur[song.sel];
    lys.reset_all_slot = (uint8_t)(slot % NENT);
    lys.reset_all_track = song.sel;
    lys.reset_all_at = fm1_ms | 1u;
    char label[] = "CLEAR A - 8 TRACKS";
    label[6] = (char)('A' + lys.reset_all_slot);
    ui_message(label);
}

static void seq_clear_all_poll(void)
{
    if (!lys.reset_all_at)
        return;
    if (!(kb_layer & SEQ_ALL_KEY) || layer_open() != LAYER_SEQ || song.sel != lys.reset_all_track || eu.on) {
        lys.reset_all_at = 0;
        ui_message("CLEAR ALL CANCELLED");
    } else if (fm1_ms - (lys.reset_all_at & ~1u) >= SEQ_ALL_HOLD_MS) {
        lys.reset_all_at = 0;
        if (transport_busy())
            ui_message("STOP TO CLEAR ALL");
        else
            seq_clear_column(lys.reset_all_slot);
    }
}

/* REC: CLEAR, the selected track's steps and automation (and ARP's latched chord): one undo (SAVE held), no dialog,
 * as SEQ TOOLS' CLEAR (up to 1.1.4 REC held on SEQ asked first, CF_CLEAR_SEQ). Its own undo copy (not a sound load's) */
static void rec_clear(void)
{
    track_t *t = TSEL;
    if (seq_is_empty(t) && !motion_count(t)) {
        ui_message("NOTHING TO CLEAR");
        return;
    }
    undo.keep = 0;
    undo_label = "CLEAR";
    load_begin(t, UNDO_PAT);
    track_defaults_steps(t);
    load_end(t);
    t->nheld = 0;
    t->arp_phys = 0;
    ui_message("SEQUENCE CLEARED");
    if (!lys.loaded)                                    /* (the first one: how to take it back) */
        str_cpy(ui.msg2, "HOLD [SAVE]: UNDO", sizeof ui.msg2);
    lys.loaded = 1;
    ui.force = 1;
}
/* (Skiff U2, DESIGN.md rule 7) REC's CLEAR key held for the hold time (MENU > HOLD) clears; let go sooner: nothing */
static void rec_clear_poll(void)
{
    if (!lys.clr_t)
        return;
    if (!(kb_layer & 1u) || layer_open() != LAYER_REC) {
        lys.clr_t = 0;
        return;
    }
    if (fm1_ms - (lys.clr_t & ~1u) >= HOLD_MS[settings_hold % 4u]) {
        lys.clr_t = 0;
        rec_clear();
    }
}
/* REC's settings, row r (G3 A3 B3, KNOB 1..3): CLICK, COUNT-IN, CLICK LEVEL as MENU > AUDIO steps them (ui.c rp_*) */
static const uint8_t RL_F[3] = {RP_CLICK, RP_COUNTIN, RP_LEVEL};
static const char *const RL_LABEL[3] = {"CLICK", "COUNT-IN", "CLICK LVL"};   /* ((Skiff U4, R9) MENU > AUDIO's names, as
                                                       * they fit Barlow's card and key cell) */
static const uint8_t RL_ICON[3] = {ICON_TEMPO, ICON_TIME, ICON_LEVEL};
static const char *rl_name(uint32_t r, uint32_t v)
{
    return (r == 0u ? CLICK_N : r == 1u ? COUNTIN_N : CLKLVL_N)[v % 3u];
}
static void rec_set(uint32_t r, uint32_t v)
{
    if (v == rp_get(RL_F[r]))
        return;
    rp_put(RL_F[r], v);
    lys.rp_dirty = 1;
}
static void rec_key(uint32_t p)
{
    uint32_t r = p - 1u, v;
    if (p == 0u) {                                      /* ((Skiff U2) CLEAR: held for the hold time, rec_clear_poll) */
        lys.clr_t = fm1_ms | 1u;
        ui_message("HOLD: CLEAR");
        return;
    }
    v = (rp_get(RL_F[r]) + 1u) % 3u;                    /* the next value, round */
    rec_set(r, v);
    ui_say(r == 2u ? "CLICK LEVEL " : r ? "COUNT-IN " : "CLICK ", rl_name(r, v));
}

/* (Skiff) PERFORM's SELECT (ls) the LAUNCH point, PRESETS (lp) the KEYS: said, kept with the settings when the layer
 * lets go; the footer says them */
static void perf_encoders(int32_t ls, int32_t lp)
{
    uint32_t m = (uint32_t)clamp((int32_t)launch_mode() + ls, 0, LAUNCH_N - 1);
    if (ls && m != launch_mode()) {
        ui_launch = (uint8_t)m;
        lys.rp_dirty = 1;
        ui.force = 1;
        ui_say("LAUNCH ", LAUNCH_NAME[m]);
    }
    if (lp && (lp > 0) != perf_scenes()) {
        ui_perf_keys = (uint8_t)(lp > 0);
        lys.rp_dirty = 1;
        ui.force = 1;
        ui_say("KEYS ", perf_scenes() ? "SCENE" : "TRACK");
    }
}

/* (Skiff) PERFORM, KEYS TRACK: entry p's key: the selected track to that entry of its list (bank.c entry_ui) */
static void perf_slot_key(uint32_t k, uint32_t p)
{
    (void)k;
    entry_ui(p);
}

/* The first eight black keys mute T1–T8. Their immediate white neighbours solo
 * the same tracks. The last three white keys are ALL, FILL and TAP. */
enum { GW_ALL = 8, GW_FILL, GW_TAP, GW_NONE };
static const uint8_t GLO_W[] = {0, 1, 2, GW_NONE, 3, 4, GW_NONE, 5, 6, 7,
                               GW_NONE, GW_NONE, GW_NONE, GW_ALL, GW_FILL, GW_TAP};
static uint32_t glo_white(uint32_t k)
{
    uint32_t p = key_place(k);
    return !key_black(k) && p < sizeof GLO_W ? GLO_W[p] : GW_NONE;
}

/* (2.0, D 1A, design/mockups/v2_mutes.png) the black keys 1..6 in GLO, FX and PERFORM: each track's MUTE, alike. A tap
 * (let go within MENU's HOLD) toggles it, latched and saved: GLO's and FX's at once, PERFORM's at its LAUNCH point
 * (mute_switch); held, the track is cut while it is: FX's from the press (the ISR: perf_held), GLO's from the press,
 * PERFORM's once it is a hold (perf_cut). Each pass, before the layer's keys */
static struct { uint8_t dn, cut, l[NTRK]; uint32_t t0[NTRK]; } mk __attribute__((section(".pool")));
static int trk_cut(uint32_t t)                          /* track t cut by a black key held now */
{
    return !perf_kill && ((((perf_held >> PF_M1) | perf_cut) >> t) & 1u);
}
static void mute_keys(uint32_t l, uint32_t keys)
{
    uint32_t k, p, b, now = fm1_ms, hold = HOLD_MS[settings_hold % 4u];
    for (k = 0; k < 27u; k++) {
        if (!key_black(k) || (p = key_place(k)) >= NTRK)
            continue;
        b = 1u << p;
        if (((keys >> k) & 1u) && (l == LAYER_GLO || l == LAYER_FX || l == LAYER_PERF)) {
            mk.dn |= (uint8_t)b;
            mk.l[p] = (uint8_t)l;
            mk.t0[p] = now;
            if (l == LAYER_GLO)
                mk.cut |= (uint8_t)b;
        } else if ((mk.dn & b) && !((kb_layer >> k) & 1u)) {   /* let go: a tap toggles it */
            mk.dn &= (uint8_t)~b;
            mk.cut &= (uint8_t)~b;
            if (now - mk.t0[p] < hold && mk.l[p] == LAYER_PERF)
                mute_switch(p);
            else if (now - mk.t0[p] < hold)
                trk[p].p[P_MUTE] = (int16_t)!trk[p].p[P_MUTE];
        } else if ((mk.dn & b) && mk.l[p] == LAYER_PERF && now - mk.t0[p] >= hold) {
            mk.cut |= (uint8_t)b;                       /* (PERFORM: a hold cuts from here) */
        }
    }
    perf_cut = mk.cut;
}

/* a key pressed in layer l (k: 0 = F3 .. 26 = G5) */
static void layer_key(uint32_t l, uint32_t k)
{
    uint32_t p = key_place(k), i, w = glo_white(k);
    if (l == LAYER_GLO) {                               /* ((2.0) the black keys' MUTE: mute_keys) */
        if (w < NTRK) {
            lys.solo |= 1u << k;                        /* (layer_masks: perf_solo while held) */
        } else if (w == GW_ALL) {
            lys.solo = 0;
            for (i = 0; i < NTRK; i++)
                trk[i].p[P_MUTE] = 0;
        } else if (w == GW_FILL) {
            lys.fill |= 1u << k;                        /* (Skiff) D4: FILL while held (layer_keys) */
        } else if (w == GW_TAP) {
            glo_tap();
        }
    } else if (l == LAYER_SCL) {
        TSEL->p[P_ROOT] = (int16_t)((k + 5u) % 12u);    /* the key's note name (F3 = F) */
    } else if (l == LAYER_EDIT && !key_black(k)) {
        if (p < LY_INIT)
            edit_family(p);
        else if (p == LY_INIT)
            confirm_open(CF_INIT_SOUND, song.sel);      /* (the dialog closes the layer) */
    } else if (l == LAYER_EDIT && p < NLANE && drum_track(TSEL)) {   /* ((1.3, from Felucca 1.4.1) black keys 1..8:
                                                         * the lanes' MUTE, latched (eng_drum.c drum_mute) */
        drum_mute[song.sel] ^= (uint8_t)(1u << p);
        ui_say(drum_lane_name(TSEL, p), (drum_mute[song.sel] >> p) & 1u ? " MUTED" : " ON");
        ui.force = 1;
    } else if (l == LAYER_REC) {
        if (!key_black(k) && p < 4u)
            rec_key(p);
    } else if (l == LAYER_PERF) {                       /* (Skiff) the white keys: the list's A..P or (KEYS SCENE) the */
        if (!key_black(k) && p < BANK_SLOTS && perf_scenes())   /* scenes, SAVE held: stored (bank.c); the black */
            scene_key(p, (fm1_in.buttons >> panel.btn[B_SAVE]) & 1u);   /* keys 1..6 the tracks' MUTEs, staged */
        else if (!key_black(k) && p < BANK_SLOTS)
            perf_slot_key(k, p);                        /* ((2.0) the black keys: mute_keys) */
    } else if (l == LAYER_SEQ) {
        if (!key_black(k) && p == SEQ_ALL_PLACE)
            seq_clear_all_start();
        else if (!key_black(k))
            tools_key(p);
        else if (p < NLANE && drum_track(TSEL)) {
            ui.lane = (uint8_t)p;                       /* (black keys 1..8: the lane, as on the grid) */
            eu_lane(TSEL);                              /* ((1.3) EUCLID open: on that lane now) */
        }
    }
}
/* each pass: the keys the layer got now; then GLO's solo: the tracks of its keys still held (perform.c) */
static void layer_keys(uint32_t keys)
{
    uint32_t k, l = layer_open() ? layer_open() : ui.layer, s = 0;
    seq_clear_all_poll();
    mute_keys(l, keys);
    for (k = 0; keys && k < 27u; k++)
        if ((keys >> k) & 1u)
            layer_key(l, k);
    lys.solo &= kb_layer;
    for (k = 0; k < 27u; k++)
        if ((lys.solo >> k) & 1u)
            s |= 1u << glo_white(k);               /* ((Skiff phase 6) the track it solos) */
    perf_solo = (uint8_t)s;
    lys.fill &= kb_layer;                               /* (Skiff) FILL: while its key is held in the layer */
    seq_fill = lys.fill != 0u;
}

/* KNOB 1..4 in the open layer: its own (FX macros, GLO levels, EDIT's sound), else the four of its page */
static void layer_level(track_t *t, uint32_t k, int32_t s)
{
    const param_desc_t *d = &TP[P_LEVEL];
    int16_t *vp = &t->p[P_LEVEL];
    *vp = (int16_t)clamp(*vp + accel(EN_K1 + k, s, d->max - d->min), d->min, d->max);
    motion_capture(t, P_LEVEL, *vp);
    ml_knob(t, P_LEVEL);
}

typedef struct {
    int16_t original, last;
    uint8_t id, engine, valid;
} perform_param_t;

static perform_param_t perform_params[NTRK][4] __attribute__((section(".pool")));

static uint32_t perform_param_id(const track_t *t, uint32_t k)
{
    return k == 3u ? P_LEVEL : throw_id(t, k);
}

static int perform_param_matches(const perform_param_t *p, const track_t *t, uint32_t id)
{
    return p->valid && p->engine == t->eng_req && p->id == id && p->last == t->p[id];
}

static int perform_params_changed(void)
{
    for (uint32_t k = 0; k < 4u; k++)
        if (perform_param_matches(&perform_params[song.sel][k], TSEL, perform_param_id(TSEL, k)))
            return 1;
    return 0;
}

static void perform_reset_params(void)
{
    for (uint32_t k = 0; k < 4u; k++) {
        uint32_t id = perform_param_id(TSEL, k);
        perform_param_t *p = &perform_params[song.sel][k];
        if (perform_param_matches(p, TSEL, id))
            TSEL->p[id] = p->original;
        p->valid = 0;
    }
    ui_message("PARAMETERS RESET");
}

static void layer_knob(uint32_t k, int32_t s)
{
    uint32_t l = layer_open();
    if (l == LAYER_FX) {                                /* perform.c perf_k, not recorded ((2.0, D 2A) back when the layer */
        if (k == 3u && !perf_harm_on())                 /* closes, over RETURN; OCT+ keeps: ui.c mom_*) */
            s = -s;                                     /* DEPTH (100 - perf_k[3]) rises to the right (#40: it fell) */
        (void)mom_hold(&perf_k[k], 0);
        perf_k[k] = (int16_t)clamp(perf_k[k] + accel(EN_K1 + k, s, k ? 100 : 200), k ? 0 : -100, 100);   /* (#126) */
    } else if (l == LAYER_GLO) {                        /* the four tracks' LEVEL (T1..T4; (Skiff phase 6) the four
                                                         * shown: ui.c trk_top), recorded as on MIXER */
        layer_level(&trk[trk_top() + k], k, s);
    } else if (l == LAYER_EDIT) {                       /* ENG, No., FAV */
        if (k == 0u)
            edit_load(eng_step_for(song.sel, TSEL->eng_req, s), 0);
        else if (k == 1u)
            edit_load(NENGINES, s);
        else if (k == 2u)
            preset_mark(s > 0);
    } else if (l == LAYER_PERF) {
        uint32_t id = perform_param_id(TSEL, k);
        const param_desc_t *d = track_desc(TSEL, id);
        perform_param_t *p = &perform_params[song.sel][k];
        if (!perform_param_matches(p, TSEL, id))
            *p = (perform_param_t){.original = TSEL->p[id], .id = (uint8_t)id, .engine = TSEL->eng_req};
        if (k == 3u)
            layer_level(TSEL, k, s);
        else
            TSEL->p[id] = (int16_t)param_turn(d, TSEL->p[id], accel(EN_K1 + k, s, d->max - d->min));
        p->last = TSEL->p[id];
        p->valid = p->last != p->original;
        ui.hot_col = (uint8_t)k;
        ui.hot_t = 40;
    } else if (l == LAYER_REC) {                        /* CLICK, COUNT-IN, LEVEL, (Skiff) FEEL */
        if (rec_grid_page) {
            uint8_t prefs = rec_prefs_for(song.sel);
            if (k == 0u)
                prefs = (uint8_t)((prefs & 0xF0u) |
                    clamp((int32_t)(prefs & 15u) + s, 0, (int32_t)NELEM(REC_GRID_NAME) - 1));
            else if (k == 1u)
                prefs = (uint8_t)(s < 0 ? prefs | REC_TRACK_FEEL : prefs & ~REC_TRACK_FEEL);
            else if (k == 3u)
                track_select((uint32_t)clamp((int32_t)song.sel + s, 0, NTRK - 1));
            if (k < 2u && prefs != rec_prefs_for(song.sel)) {
                rec_track_prefs[song.sel] = prefs;
                lys.rp_dirty = 1;
            }
        } else if (k < 3u)
            rec_set(k, (uint32_t)clamp((int32_t)rp_get(RL_F[k]) + s, 0, 2));
    } else if (l == LAYER_SEQ && eu.on) {               /* ((1.3) EUCLID: HITS STEPS ROT; KNOB 4: none) */
        eu_knob(TSEL, k, s);
    } else if (l) {                                     /* the page's knobs, wherever the page is */
        uint8_t h = ui.home, pg = ui.page;
        ui.home = 0;
        ui.page = (uint8_t)page_first(LAYERS[l].fam);
        page_over = ly_page(l);
        edit_param(k, s);
        page_over = 0;
        ui.home = h;
        ui.page = pg;
    }
}

/* PLAY with GLO held open: RESTART (from the top, playing on); stopped: PLAY. (Skiff phase 5) SONG on: each track
 * from its list's first entry (bank.c song_top). 1 = done here. GLO's lock is not held: PLAY starts and stops there
 * as in every layer (Felucca #183: locked, PLAY restarted and never stopped) */
static int layer_play(void)
{
    if (layer_open() != LAYER_GLO || !(fm1_in.buttons & ly_bit(LAYER_GLO)))
        return 0;
    if (song.g[G_CLOCK] && song.playing) {
        ui_message("RESTART: CLOCK EXTERNAL");
        return 1;
    }
    if (lists.song)
        song_top();
    else if (song.playing)
        transport_req = 3;
    else {
        chain_top();                                  /* ((2.0, #44) a chain from its first part) */
        transport_req = 1;
    }
    return 1;
}

/* OCT- / OCT+ pressed with a SET layer's button down (FX LATCH: FX's) before it opened: a combo, so it opens now,
 * before oct_taps and layer_oct read it (else the press was nobody's: no put back / ALL OFF, no octave) */
static void layer_oct_open(uint32_t pressed)
{
    uint32_t l = ui.ly, oct = 1u << panel.btn[B_OCTDN] | 1u << panel.btn[B_OCTUP];
    if ((pressed & oct) && l && !(ui.ly_t0 & (LY_OPEN | LY_DEAD)) && layer_held() &&
        (LAYERS[l].kind == LK_SET || (l == LAYER_FX && perf_latch_on))) {
        ui.ly_t0 |= LY_OPEN | LY_COMBO;
        layer_opened(l);
    }
}

/* OCT- / OCT+ in a SET layer: no octave; OCT- let go puts back what the layer changed. Returns the taps left */
static uint32_t layer_oct(uint32_t pressed, uint32_t oct)
{
    uint32_t dn = panel.btn[B_OCTDN], up = panel.btn[B_OCTUP], k, mine;
    if (layer_set_open())
        lys.oct |= (uint8_t)(((pressed >> dn) & 1u) | ((pressed >> up) & 1u) << 1);
    mine = oct & lys.oct;
    if ((mine & 3u) && eu.on && lys.l == LAYER_SEQ) {   /* ((1.3) EUCLID open: OCT+ applies, OCT- cancels; only that) */
        eu_close(&trk[lys.trk % NTRK], mine & 2u);
        ui_message(mine & 2u ? "EUCLID APPLIED" : "EUCLID CANCELLED");
        ui.force = 1;
        mine = 0;
    }
    if ((mine & 2u) && lys.l == LAYER_EDIT && drum_track(TSEL)) {   /* ((2.0.1, #54) OCT+: every lane on at once, as a */
        drum_mute[song.sel] = 0;                        /* build-up's release (the black keys mute them one by one) */
        ui_message("ALL LANES ON");
        ui.force = 1;
    }
    if (mine & 1u) {                                    /* OCT-: back to when it opened */
        if (lys.l == LAYER_FX) {                        /* (FX LATCH) every effect and macro off */
            perf_latched = 0;
            perf_k[0] = perf_k[1] = perf_k[2] = perf_k[3] = 0;
            (void)mom_keep(0, 1);                       /* ((2.0) nothing of them to come back) */
            ui_message("FX ALL OFF");
        } else if (lys.l == LAYER_EDIT) {
            if (lys.loaded)
                undo_step(0);                           /* (the copy from its first load) */
            lys.loaded = 0;
            ui.preset_lists[lys.trk % NTRK] = lys.preset_list;
            ui.force = 1;
            memcpy(drum_mute, lys.mute, sizeof drum_mute);   /* ((1.3) the lanes' mutes as it opened) */
        } else if (lys.l == LAYER_SEQ || lys.l == LAYER_REC) {
            track_t *t = &trk[lys.trk % NTRK];
            if (lys.loaded)                             /* (SAVE held then: the tools / the clear again) */
                (void)tl_put_back(t);
            if (lys.l == LAYER_REC) {                   /* REC: the settings as it opened */
                lys.rp_dirty |= (uint8_t)(ui_rec_prefs != (uint8_t)lys.v[0]);
                lys.rp_dirty |= memcmp(rec_track_prefs, lys.rec_tracks, sizeof lys.rec_tracks) != 0;
                ui_rec_prefs = (uint8_t)lys.v[0];
                memcpy(rec_track_prefs, lys.rec_tracks, sizeof lys.rec_tracks);
                rp_apply();
            } else {
                for (k = 0; k < 4u; k++)
                    t->p[P_SLEN + k] = lys.v[k];
            }
            lys.loaded = 0;
            lys.cook = 0;
            ui_message(lys.l == LAYER_REC ? "UNDONE: REC" : "UNDONE: SEQUENCE");   /* ((Skiff U4) was PUT BACK) */
        } else if (lys.l == LAYER_PERF) {               /* (Skiff) nothing staged to come in: patterns, mutes */
            ui_message(stage_cancel() ? "CANCELLED" : "NOTHING STAGED");
        } else if (lys.l == LAYER_GLO) {
            for (k = 0; k < NTRK; k++) {
                trk[k].p[P_MUTE] = lys.v[k];
                trk[k].p[P_LEVEL] = lys.v[NTRK + k];
            }
            song.g[G_BPM] = lys.v[LY_BPM];
            ui_message("UNDONE: MIX");
        } else {
            for (k = 0; k < 4u; k++)
                trk[lys.trk % NTRK].p[P_ROOT + k] = lys.v[k];
            trk[lys.trk % NTRK].p[P_CHRD] = lys.v[4];
            trk[lys.trk % NTRK].p[P_VOIC] = lys.v[5];
            ui_message("UNDONE: SCALE");
        }
    }
    lys.oct &= (uint8_t)(((fm1_in.buttons >> dn) & 1u) | ((fm1_in.buttons >> up) & 1u) << 1);
    return oct & ~mine;
}

/* --------------------------------------------------------- the LEDs --- */
/* lit = in effect now (held, the value, a track sounding), breathing = can be pressed (*br: dark .. ~60 % and
 * back, ~1.1 s, hal/fm1_input.h fm1_led_breath; was a hard 250 ms blink, #119), dark = nothing there */
static int trk_cut(uint32_t t);
static int glo_sounding(uint32_t t)
{
    return !trk[t].p[P_MUTE] && !trk_cut(t) && (!perf_solo || ((perf_solo >> t) & 1u));
}
static uint32_t glo_mutes(void)                     /* (Skiff phase 6) every track's MUTE, a bit each */
{
    uint32_t k, m = 0;
    for (k = 0; k < NTRK; k++)
        m |= (uint32_t)(trk[k].p[P_MUTE] != 0) << k;
    return m;
}
static int rec_clearable(void) { return !seq_is_empty(TSEL) || motion_count(TSEL) != 0u; }
static uint32_t layer_leds(uint32_t *br)
{
    uint32_t k, m = 0, n = 0, l = ui.layer, held = perf_held | perf_latched, ok = perf_avail();
    uint32_t mask = scale_mask(TSEL), root = (uint32_t)TSEL->p[P_ROOT] % 12u;
    for (k = 0; k < 27u; k++) {
        uint32_t p = key_place(k), b = (uint32_t)key_black(k), on = 0, can = 0, e;
        if (l == LAYER_FX && b) {                       /* ((2.0, D 1A) the black keys as GLO's: sounding lit) */
            on = p < NTRK && glo_sounding(p);
        } else if (l == LAYER_FX) {                     /* effects breathe, held lit, a too-long REPEAT dark */
            e = perf_key(k);
            can = e < PF_N && ((ok >> e) & 1u);
            on = can && ((held >> e) & 1u);
        } else if (l == LAYER_GLO) {                    /* sounding lit; SOLO held lit, the others, C4, F4 breathe; */
            e = glo_white(k);                           /* (Skiff) F4 FILL too */
            on = b ? p < NTRK && glo_sounding(p) : (e < NTRK && ((lys.solo >> k) & 1u)) || (e == GW_FILL && seq_fill);
            can = !b && (e < NTRK || e == GW_ALL || e == GW_FILL || (e == GW_TAP && !song.g[G_CLOCK]));
        } else if (l == LAYER_SCL) {                    /* the root lit, the scale's notes breathe */
            e = (k + 5u + 12u - root) % 12u;
            on = e == 0u;
            can = (mask >> e) & 1u;
        } else if (l == LAYER_EDIT && !b) {
            on = p < LY_INIT && edit_list_mode(p) == list_mode();
            can = p <= LY_INIT;
        } else if (l == LAYER_EDIT && p < NLANE && drum_track(TSEL)) {   /* ((1.3) the lanes: sounding lit, muted
                                                         * breathe, as GLO's track mutes) */
            on = !((drum_mute[song.sel] >> p) & 1u);
            can = 1;
        } else if (l == LAYER_SEQ) {                    /* the tools breathe; DRUM: the lane */
            on = (b && p < NLANE && drum_track(TSEL) && p == ui.lane) || (!b && p == TL_EUCLID && eu.on);   /* lit,
                                                         * the other lanes breathe ((1.3) EUCLID open: lit) */
            can = b ? p < NLANE && drum_track(TSEL) : tl_cell(TSEL, p) || (p == SEQ_ALL_PLACE && !eu.on);
        } else if (l == LAYER_PERF && b && p < NTRK) {  /* (Skiff) a track sounding lit; a muted one, a staged one */
            on = !trk[p].p[P_MUTE] && !mute_arm[p] && !trk_cut(p);   /* breathe ((2.0) a hold cutting too) */
            can = 1;
        } else if (l == LAYER_PERF && !b && p < BANK_SLOTS && perf_scenes()) {   /* (Skiff) the scene recalled or */
            on = p == scene_now();                      /* stored last lit, the other kept ones breathe */
            can = scene_used(p);
        } else if (l == LAYER_PERF && !b && p < BANK_SLOTS) {   /* (Skiff) the slot playing and the one staged lit, */
            on = p == bank_cur(song.sel) || p == bank_next(song.sel);   /* the other kept ones breathe */
            can = ent_on(song.sel, p);                   /* ((#28) a REST too: the track drops out) */
        } else if (l == LAYER_REC && !b && p < 4u) {    /* a setting on (LEVEL: the click on) lit, the rest breathe; */
            on = p == 1u ? rp_get(RP_CLICK) != 0u : p == 2u ? rp_get(RP_COUNTIN) != 0u :   /* CLEAR: something to */
                 p == 3u && rp_get(RP_CLICK) != 0u;    /* clear */
            can = p ? 1u : rec_clearable();
        }
        m |= on << k;
        n |= (can & !on) << k;
    }
    if (br)
        *br = n;
    return m;
}

/* ------------------------------------------------------ the overlay --- */
/* One template: the header names the button and the kind ("[GLO] SET"); the cards are KNOB 1..4; the panel the
 * map of the keys as cells (the key's note name, a Fukiai icon, a name; FX's effects: the icon alone, 24 px, the
 * REPEATs' division in the other corner); the footer the keycaps. A cell: RAISE
 * (can be pressed), the selection's fill (the value now, SET), the accent (held, HOLD), DIM (cannot now: pressing
 * it says why), KEY (a muted track, as the MUTE badge); HOLD cells in a SET layer: a corner triangle */
static const char *const PF_DIV[3] = {"8", "16", "32"};   /* the REPEATs' 1/8 1/16 1/32 ((Skiff phase 9) 30 px cells: the
                                                           * "1/" off); the other effects: their icon alone */
static const uint8_t PF_ICON[PF_M1] = {ICON_X_REPEAT, ICON_X_REPEAT, ICON_X_REPEAT, ICON_X_REVERSE, ICON_CUTOFF,
    ICON_X_HPF, ICON_X_TSTOP, ICON_X_FREEZE, ICON_X_OCT_UP, ICON_X_OCT_DN, ICON_X_CHOP, ICON_X_DUB, ICON_X_FLANGE,
    ICON_X_VINYL};
static const char W_NOTE[16] = {'F', 'G', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'A', 'B', 'C', 'D', 'E', 'F', 'G'};
static const char B_NOTE[] = {'F', 'G', 'A', 'C', 'D', 'F', 'G', 'A'};
_Static_assert(sizeof B_NOTE >= NTRK, "a black key's name for every track");
#define LC_X(c) (6 + 58 * (int32_t)(c))                 /* cell column c: 54 px wide, 4 px apart */
#define LC_W 54
#define LC_H 42                                          /* the big cells: rows at y 4 and 50 */
#define LF_X(c) (6 + 33 * (int32_t)(c))                 /* FX: the 14 effects, 7 a row (an octave each: F3 .. E4, F4 ..
                                                         * E5), 30 px: x 6 .. 234 ((Skiff phase 9) were 10, 5 a row) */
#define LF_W 30
#define LF_ROW 7u
#define LM_Y 96                                          /* the black keys' row (22 px) */
#define LM_H 22
enum { LS_OFF, LS_SEL, LS_HELD, LS_WAIT, LS_DIM, LS_MUTE };

static int32_t lc_w = LC_W;                              /* the cells' width (FX's effects: LF_W) */
static uint16_t lc_fill(uint32_t st, uint16_t *ink)
{
    *ink = st == LS_DIM ? T_DIM : T_THEME;
    if (st == LS_MUTE) {
        *ink = T_INK;
        return T_KEY;
    }
    if (st == LS_HELD) {
        *ink = T_BG;
        return T_ACCENT;
    }
    if (st == LS_SEL || st == LS_WAIT) {
        *ink = T_INK;
        return T_THEME;
    }
    return T_RAISE;
}
/* a cell at x, y, h px high: big (h > 30, FX's effects, LF_W wide) the icon alone, 24 px, under the note, a name in
 * the top right corner; over 24 px (SEQ TOOLS) compact, the icon at a side; else a key's row (the EDIT layer): the
 * icon and the name on its middle. ((Skiff phase 6) GLO's and FX's track cells: l6_*) */
/* a cell's box: its fill on the panel ((Skiff) a RAISE, idle, cell is a SURF tile on the ground). Cells are 4 px
 * apart from x 6, y 4. Returns the fill */
static uint16_t lc_box(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t fill)
{
    if (fill == T_RAISE)
        fill = T_SURF;
    cv_rrect(x, y, w, h, ux.crypt ? 0 : 4, fill, cv_bg);
    if (ux.crypt && fill == T_SURF) {                    /* (Skiff C3) CRYPT: an idle cell stone, framed */
        cr_stone(x, y, w, h, fill);
        cv_frame(x, y, w, h, T_LINE);
    }
    return fill;
}
static void lcell(int32_t x, int32_t y, int32_t h, const char *note, uint32_t icon, const char *name, uint32_t st)
{
    uint16_t ink, fill = lc_fill(st, &ink), idle = fill == T_RAISE;
    int32_t in = lc_w < 40 ? 3 : 5;                        /* ((Skiff phase 9) FX's 30 px cells: the note and a REPEAT's
                                                            * division 3 px in, apart) */
    fill = lc_box(x, y, lc_w, h, fill);
    if (note)                                              /* (a 28 px cell, SEQ TOOLS: y + 1, Barlow's 10 px capitals */
        cv_text_on(x + in, y + (h > 24 && h <= 30 ? 1 : 3), &AF_S, note, idle ? T_MID : ink, fill);   /* clear its name) */
    if (h > 30) {
        GFX_HOOK_ALIGN(x, 0, x + lc_w, 0, AL_H, "FX cell icon centred across");
        cv_icon_on(x + (lc_w - 24) / 2, y + h - 25, 24, icon, ink, fill);   /* (its ink: rows 3 .. 20 of 24) */
        if (name)
            cv_text_r(x + lc_w - in, y + 3, &AF_S, name, ink, fill);
    } else if (h > 24) {                                   /* SEQ TOOLS: the icon (12 px) over the name, 2 px between, */
        int32_t b[4], t;                                   /* centred by ink (the icon at a side) */
        icon_ink(12, icon, b);
        t = y + HALF_UP(h - (14 + AF_S_CAP_H - b[1])) - b[1];
        GFX_HOOK_ALIGN(0, y, 0, y + h, AL_V | AL_N(2), "layer cell icon + name centred up/down");
        cv_icon_on(note ? x + lc_w - 17 : x + 5, t, 12, icon, ink, fill);
        GFX_HOOK_ALIGN(x, 0, x + lc_w, 0, AL_H | AL_PASS, "layer cell icon / name centred across");
        cv_text_in(x, t + 14 - AF_S_CAP_Y, lc_w, &AF_S, name, ink, fill);
    } else {                                               /* a key's row: the icon and the name on its middle */
        GFX_HOOK_ALIGN(0, y, 0, y + h, AL_V, "layer key cell icon / name centred up/down");
        cv_icon_in(x + 3, y, 0, h, 12, icon, ink, fill);
        GFX_HOOK_ALIGN(0, y, 0, y + h, AL_V, "layer key cell icon / name centred up/down");
        cv_text_r(x + lc_w - 3, y + CAP_IN(S, h), &AF_S, name, ink, fill);
    }
}
static void bnote(char *n, uint32_t t) { n[0] = B_NOTE[t]; n[1] = '#'; n[2] = 0; }
/* FX mute cells and GLO's utility row. */
#define L6_X(c) (4 + 29 * (int32_t)(c))
#define L6_W 27
#define L6_WIDE 75
/* The white key's octave changes at C. The paired black key shares it. */
static void glo_note(char *note, uint32_t white, int sharp)
{
    note[0] = W_NOTE[white];
    if (sharp)
        note[1] = '#';
    note[1 + sharp] = (char)('3' + (white + 3u) / 7u);
    note[2 + sharp] = 0;
}

static void glo_pair_cell(int32_t x, int32_t y, const char *label, const char *note, int solo, int active)
{
    uint16_t fill = active ? T_ACCENT : solo ? T_KEY : T_SURF;
    uint16_t ink = active || solo ? T_INK : T_TEXT;
    fill = lc_box(x, y, 26, 25, fill);
    cv_text_in(x, y + 3, 26, &AF_S, label, ink, fill);
    cv_text_in(x, y + 14, 26, &AF_S, note, ink, fill);
}
static void l6_mute(uint32_t k, uint32_t st, uint32_t icon)   /* (Skiff U2, R10) icon: GLO's MUTE (latched), FX's
                                                                * CUT while held: the gate, alone (as FX's white keys'
                                                                * effects: no room for it beside the note's name) */
{
    int32_t x = L6_X(k), y = LM_Y, b[4], cut = icon != ICON_MUTE;
    uint16_t ink, fill = lc_fill(st, &ink), idle = fill == T_RAISE;
    char n[3], d[2] = {(char)('1' + k), 0};
    bnote(n, k);
    fill = lc_box(x, y, L6_W, LM_H, fill);
    GFX_HOOK_ALIGN(0, y, 0, y + LM_H, AL_V, "layer key cell icon / name centred up/down");
    if (!cut)
        cv_text_on(x + 3, y + CAP_IN(S, LM_H), &AF_S, n, idle ? T_MID : ink, fill);
    /* the speaker's ink from x + 18 (its ink its lint box: no room for its empty margins), the gate's from x + 11;
     * the number in M (every style: no room for a cushion), the track's colour, its ink ending at x + 34 */
    icon_ink(12, icon, b);
    icon_lint_ink = 1;
    GFX_HOOK_ALIGN(0, y, 0, y + LM_H, AL_V, "layer key cell icon / name centred up/down");
    cv_icon_in(x + (cut ? 8 : 12) - b[0], y, 0, LM_H, 12, icon, ink, fill);
    icon_lint_ink = 0;
    text_ink(&AF_M, d, b);
    GFX_HOOK_ALIGN(0, y, 0, y + LM_H, AL_V, "layer key cell icon / name centred up/down");
    cv_text_on(x + L6_W - 2 - b[2], y + CAP_IN(M, LM_H), &AF_M, d, idle ? T_TRK(k) : ink, fill);
}

static void layer_fx(void)
{
    uint32_t held = perf_kill ? 0u : perf_held | perf_latched, act = perf_act, ok = perf_avail(), e;
    char n[3] = {0, 0, 0};
    lc_w = LF_W;
    for (e = 0; e < PF_M1; e++) {                       /* the effects of the white keys F3 .. E5, an octave a row */
        if (e % LF_ROW == 0u)
            GFX_HOOK_ALIGN(0, 0, 240, 0, AL_H | AL_CELLS | AL_N(LF_ROW), "FX cells' row centred");
        uint32_t st = !((ok >> e) & 1u) ? LS_DIM : !((held >> e) & 1u) ? LS_OFF : (act >> e) & 1u ? LS_HELD : LS_WAIT;
        n[0] = W_NOTE[e];
        lcell(LF_X(e % LF_ROW), e < LF_ROW ? 4 : 50, LC_H, n, PF_ICON[e], e < 3u ? PF_DIV[e] : 0, st);
    }
    lc_w = LC_W;
    for (e = 0; e < NTRK; e++)                          /* ((2.0, D 1A) the black keys 1..6 as GLO's: MUTE, a hold */
        l6_mute(e, trk[e].p[P_MUTE] || trk_cut(e) ? LS_MUTE : LS_OFF, ICON_MUTE);   /* cutting) */
}
static void layer_glo(void)
{
    static const char *const tools[] = {"ALL", "FILL", "TAP"};
    uint32_t p;
    for (p = 0; p < sizeof GLO_W; p++) {
        uint32_t t = GLO_W[p];
        char note[4];
        if (t < NTRK) {
            int32_t x = 4 + 59 * (int32_t)(t % 4u), y = 2 + 41 * (int32_t)(t / 4u);
            char title[] = {'T', (char)('1' + t), 0};
            cv_text_in(x, y, 54, &AF_S, title, T_TRK(t), cv_bg);
            glo_note(note, p, 0);
            glo_pair_cell(x, y + 11, "SOLO", note, 1, (perf_solo >> t) & 1u);
            glo_note(note, p, 1);
            glo_pair_cell(x + 28, y + 11, "MUTE", note, 0, trk[t].p[P_MUTE] || trk_cut(t));
        } else if (t >= GW_ALL && t <= GW_TAP) {
            int32_t x = 4 + 78 * (int32_t)(t - GW_ALL);
            uint16_t ink, fill = lc_fill(t == GW_FILL && seq_fill ? LS_HELD :
                                        t == GW_TAP && song.g[G_CLOCK] ? LS_DIM : LS_OFF, &ink);
            fill = lc_box(x, 86, L6_WIDE, 28, fill);
            glo_note(note, p, 0);
            cv_text_in(x, 89, L6_WIDE, &AF_S, tools[t - GW_ALL], ink, fill);
            cv_text_in(x, 100, L6_WIDE, &AF_S, note, ink, fill);
        }
    }
}
static void layer_scl(void)                             /* KNOB 2's scales, 4 x 4, the one now selected */
{
    uint32_t i, sc = (uint32_t)TSEL->p[P_SCALE];
    for (i = 0; i < 16u && i <= (uint32_t)TP[P_SCALE].max; i++) {
        int32_t x = LC_X(i % 4u), y = 4 + 29 * (int32_t)(i / 4u);
        uint16_t ink, fill = lc_fill(i == sc ? LS_SEL : LS_OFF, &ink);
        fill = lc_box(x, y, LC_W, 25, fill);
        GFX_HOOK_ALIGN(x, y, x + LC_W, y + 25, AL_HV, "scale cell name centred");
        cv_text_in(x, y + CAP_IN(S, 25), LC_W, &AF_S, TP[P_SCALE].names[i], ink, fill);
    }
}
/* (Skiff) PERFORM, as the chosen mockup draws it (design/mock.py, B perform): a row per track, its number in its
 * colour, its list's 16 entries A..P across as the white keys run (F3 = A .. G5 = P): one naming a pattern its letter,
 * an empty one a dot, the one the track is on a tile of the track's colour (DIM: muted), the one staged to come in
 * that tile's outline (it wins where both: the new one is the news); at the row's end M
 * (REC) while muted, outlined while a MUTE is staged to turn (bank.c mute_switch: the M's colour says how it is now).
 * The selected track's row (the keys' and knobs' track) a band of its colour, everything on it INK, as on HOME. Under
 * the tracks SCN and the 16 scenes in the keys' order: a kept one its number in a box (TEXT while KEYS SCENE puts
 * them on the keys), the one recalled or stored last filled (outlined bright: still to come), an empty one a dot.
 * BLOCKS: on the black ground, as HOME's lanes; FLAT and LINE: on the panel, the band RAISE. (Phase 6) With six tracks
 * the rows of the four shown (ui.c trk_top), the scroll bar at x 238 over them (design/mock_p6.py perform) */
#define PR_X(p) (73 + 9 * (int32_t)(p))               /* after the sound name */
#define PR_W 12
#define PR_TH 11
#define PR_H 12
#define PR_Y(r) (2 + PR_H * (int32_t)(r))            /* eight tracks, scenes, then the shortcut */
#define PS_X(p) (28 + 13 * (int32_t)(p))              /* scene p's cell, after SCN: x 28 .. 234 */
enum { PC_EMPTY, PC_KEPT, PC_ON, PC_NEXT };
/* the cell at x in the row at y: s its letter (number), in state st; c a tile's fill and a staged one's outline and
 * letter, ink a tile's letter, kc a kept one's letter, dc an empty one's dot, g the ground; u: underlined */
static void perf_cell(int32_t x, int32_t y, int32_t width, const char *s, uint32_t st, uint16_t c, uint16_t ink, uint16_t kc,
                      uint16_t dc, uint16_t g, int u)
{
    int32_t ty = y + (PR_H - PR_TH) / 2, b[4];
    uint16_t lc = st == PC_ON ? ink : st == PC_NEXT ? c : kc, on = st == PC_ON ? c : g;
    if (st == PC_EMPTY) {
        cv_rect(x + width / 2 - 1, y + PR_H / 2 - 1, 2, 2, dc);
        return;
    }
    if (st == PC_ON)
        cv_rrect(x, ty, width, PR_TH, 2, c, g);
    else if (st == PC_NEXT)
        cv_frame(x, ty, width, PR_TH, c);
    GFX_HOOK_ALIGN(x, ty, x + width, ty + PR_TH, AL_HV, "perform cell letter centred");
    cv_text_in(x, ty + CAP_IN(S, PR_TH), width, &AF_S, s, lc, on);
    if (u) {                                          /* (under the letter's ink, 2 px below its foot) */
        text_ink(&AF_S, s, b);
        cv_rect(x + ink_in(&AF_S, s, width) + b[0], ty + PR_TH - 2, b[2] - b[0], 1, lc);
    }
}
static void layer_perf(void)
{
    int blocks = 1;                                   /* ponytail: FLAT and LINE retired (U2); U3's six rows redraw this */
    uint16_t g0 = blocks ? T_BG : T_SURF;
    uint32_t k, p, pos, now = scene_now(), act = (uint32_t)perf_scenes();
    if (blocks) {                                     /* (over the panel, as HOME: the rows are on the ground) */
        cv_rect(0, 0, 240, H_GRAPH + (int32_t)foot_less(), T_BG);
        cv_bg = T_BG;
    }
    for (pos = 0, k = 0; k < NTRK; pos++, k++) {        /* ((Skiff U3, N1) every track) */
        int32_t y = PR_Y(pos), ty = y + (PR_H - PR_TH) / 2;
        int band = k == song.sel % NTRK, cb = band && blocks, muted = trk[k].p[P_MUTE] != 0;
        uint16_t col = T_TRK(k), g = cb ? col : band ? T_RAISE : g0;
        uint16_t tile = cb ? T_INK : muted ? T_DIM : col;   /* (on the band: the M says it) */
        uint32_t cur = bank_cur(k), nx = bank_next(k);
        char d[2] = {(char)('1' + k), 0};
        if (cb && ux.neon) {                          /* (Skiff N3) NEON: the band its colour into violet; a muted row */
            uint16_t st[2] = {col, ux_mix(col, NE_VIOLET, 70)};   /* in snow */
            ne_hgrad(3, y, 234, PR_H, st, 2);
        } else if (cb)
            cv_rect(3, y, 234, PR_H, col);
        else if (band)
            cv_rrect(3, y, 234, PR_H, 3, T_RAISE, g0);
        if (ux.quark && muted)                        /* ((Skiff Q3) QUARK: a muted row dark matter, dotted) */
            qk_dotframe(16, y, 221, PR_H, cb ? T_INK : T_DIM);
        if (ux.neon && muted)                         /* ((Skiff N8) the band's too: dark snow on it) */
            ne_snow(17, y, 220, PR_H, k * 7u + 11u, cb ? col : T_BG, cb ? T_INK : T_MID);
        GFX_HOOK_ALIGN(0, y, 0, y + PR_H, AL_V, "perform track number on its row's middle");
        cv_text_on(6, y + CAP_IN(M, PR_H), &AF_M, d, cb ? T_INK : col, g);
        char name[16];
        sound_name(&trk[k], name);
        cv_free_text(17, y + CAP_IN(S, PR_H), &AF_S, name[0] ? name : "--",
                     cb ? T_INK : muted ? T_DIM : T_TEXT, g, 54);
        for (p = 0; p < BANK_SLOTS; p++) {
            char s[2] = {(char)('A' + p), 0};
            uint32_t st = p == nx ? PC_NEXT : p == cur ? PC_ON : ent_on(k, p) ? PC_KEPT : PC_EMPTY;
            if (st == PC_EMPTY)
                continue;
            perf_cell(PR_X(p), y, 8, s, st, st == PC_ON ? tile : cb ? T_INK : col, cb ? col : muted ? T_TEXT : T_INK,
                      cb ? T_INK : T_MID, cb ? ux_mix(col, T_INK, 55) : T_DIM, g, 0);   /* (on DIM: TEXT, the far end
                                                     * from it in every palette) */
        }
        if (muted || mute_arm[k]) {
            if (mute_arm[k])
                cv_frame(224, ty, PR_W, PR_TH, cb ? T_INK : T_REC);
            GFX_HOOK_ALIGN(224, ty, 224 + PR_W, ty + PR_TH, AL_HV, "perform cell letter centred");
            cv_text_in(224, ty + CAP_IN(S, PR_TH), PR_W, &AF_S, "M", cb ? T_INK : muted ? T_REC : T_MID, g);
        }
    }
    GFX_HOOK_ALIGN(0, PR_Y(NTRK), 0, PR_Y(NTRK) + PR_H, AL_V, "perform SCN on its row's middle");
    cv_text_on(6, PR_Y(NTRK) + CAP_IN(S, PR_H), &AF_S, "SCN", act ? T_TEXT : T_MID, g0);
    for (p = 0; p < BANK_SLOTS; p++) {                 /* (the 16 scenes: bank.c SCENES) */
        char s[3];
        uint32_t st = !scene_used(p) ? PC_EMPTY : p != now ? PC_KEPT : stage_pending() ? PC_NEXT : PC_ON;
        fmt_int(s, (int32_t)p + 1);
        perf_cell(PS_X(p), PR_Y(NTRK), PR_W, s, st, T_TEXT, T_INK, act ? T_TEXT : T_MID, T_DIM, g0, 0);
        if (st == PC_KEPT)                             /* (a kept one boxed, as the mockup's: after its number, the
                                                        * outline over its edge) */
            cv_frame(PS_X(p), PR_Y(NTRK) + (PR_H - PR_TH) / 2, PR_W, PR_TH, T_DIM);
    }
    const char *keep = "OCT+: RESET PARAM";
    cv_free_hint(232 - head_hint_w(keep, 120), PR_Y(NTRK + 1) + 1, keep, T_MID, T_BG, 120);
}
/* what the PERFORM map shows of every track and the scenes (draw_layer: redrawn when it changes) */
static uint32_t perf_sig(void)
{
    uint32_t k, p, h = 2166136261u, m;
    for (k = 0; k < NTRK; k++) {
        for (m = 0, p = 0; p < BANK_SLOTS; p++)
            m |= (uint32_t)ent_on(k, p) << p;
        h = (h ^ (bank_cur(k) | bank_next(k) << 8 | (uint32_t)(trk[k].p[P_MUTE] != 0) << 16 | (uint32_t)mute_arm[k] << 17))
            * 16777619u;
        h = (h ^ m) * 16777619u;
        char name[16];
        sound_name(&trk[k], name);
        for (p = 0; name[p]; p++)
            h = (h ^ (uint8_t)name[p]) * 16777619u;
        h *= 16777619u;
    }
    for (m = 0, p = 0; p < BANK_SLOTS; p++)
        m |= (uint32_t)scene_used(p) << p;
    return (h ^ m) * 16777619u;
}
static void layer_edit(void)
{
    static const char *const keys[] = {"F3", "G3", "A3", "B3", "C4", "D4", "E4", "F4", "G4", "A4"};
    _Static_assert(NELEM(keys) == LY_INIT + 1u, "one note label per EDIT key");
    uint32_t i;
    for (i = 0; i <= LY_INIT; i++) {
        int32_t x = LC_X(i % 4u), y = 4 + 32 * (int32_t)(i / 4u);
        if (i < LY_INIT) {
            uint32_t mode = edit_list_mode(i);
            lcell(x, y, 28, keys[i], mode == 1u ? ICON_X_STAR : ICON_X_FOLDER,
                  LIST_NAME[mode], mode == list_mode() ? LS_SEL : LS_OFF);
        } else
            lcell(x, y, 28, keys[i], ICON_X_WARN, "INIT", LS_OFF);
    }
    if (!drum_track(TSEL)) {
        engine_sound_row(104);
        return;
    }
    for (i = 0; i < NLANE; i++) {                       /* ((1.3) DRUM: the black keys' lane mutes in the sound's place,
                                                         * 8 cells of 25 px as SEQ TOOLS' lanes; a muted one dim) */
        int32_t x = 6 + 29 * (int32_t)i;
        uint16_t ink, fill = lc_box(x, 100, 25, 18, lc_fill((drum_mute[song.sel] >> i) & 1u ? LS_DIM : LS_OFF, &ink));
        GFX_HOOK_ALIGN(x, 100, x + 25, 118, AL_HV, "edit lane mute cell name centred");
        cv_text_in(x, 100 + CAP_IN(S, 18), 25, &AF_S, drum_lane_abbr(TSEL, i), ink, fill);
    }
}

/* SEQ TOOLS: the white keys' tools, 4 a row from F3 (the whole sequence; RANDOM COOK BEAT; the lane's), on a DRUM
 * track the lanes of black keys 1..8 under them (the selected one: the selection's fill). A melodic track: the two
 * rows of the sequence's tools only */
static const uint8_t TL_ICON[TL_N] = {ICON_CLEAR, ICON_X_REVERSE, ICON_X_LEFT, ICON_X_RIGHT, ICON_PROB, ICON_W_SH,
    ICON_DRUM, ICON_PULSE, ICON_CLEAR, ICON_X_REVERSE, ICON_STEPS, ICON_PROB, ICON_LENGTH, ICON_DIVISION};
static const char *const TL_NAME[TL_N] = {"CLEAR", "REVERSE", "SHIFT", "SHIFT", "RANDOM", "COOK", "BEAT", "EUCLID",
    "CLR", "REV", "FILL", "RND", "DOUBLE", "HALF"};
/* (1.3, from Felucca 1.5.5) EUCLID open: its cycle as cells, 16 a row (up to 4 rows), the hits filled; past it, inside
 * LEN, the cells of its repeats faint (DRUM: the lanes' row under them as always) */
static void layer_euclid(void)
{
    uint32_t len = tl_len(TSEL), i, rows = (len + 15u) / 16u, h = rows > 3u ? 16u : 22u;
    for (i = 0; i < len; i++) {
        int32_t x = 8 + 14 * (int32_t)(i % 16u), y = 6 + (int32_t)(h + 2u) * (int32_t)(i / 16u);
        uint16_t c = eu_hit(i) ? (i < eu.steps ? T_THEME : T_MID) : T_RAISE;
        cv_rrect(x, y, 12, (int32_t)h, 2, c, T_BG);
    }
}
static void layer_seq(void)
{
    uint32_t p, st = LS_OFF, drum = (uint32_t)drum_track(TSEL);
    char n[3] = {0, 0, 0}, nm[8];
    if (eu.on)
        layer_euclid();
    for (p = 0; p < TL_DOUBLE && !eu.on; p++) {
        if (!tl_cell(TSEL, p))
            continue;
        n[0] = W_NOTE[p];
        str_cpy(nm, TL_NAME[p], sizeof nm);
        if (p >= TL_LCLR) {                             /* the lane's: "CLR BD" */
            str_cpy(nm + str_len(nm), " ", 2);
            str_cpy(nm + str_len(nm), drum_lane_abbr(TSEL, ui.lane), sizeof nm - str_len(nm));
        }
        lcell(LC_X(p % 4u), 3 + 28 * (int32_t)(p / 4u), 25, n, TL_ICON[p], nm, st);
    }
    if (!eu.on) {
        uint16_t ink, fill = lc_box(6, 86, 228, 13, lc_fill(lys.reset_all_at ? LS_WAIT : LS_OFF, &ink));
        char info[] = "A / 8 TRACKS";
        info[0] = (char)('A' + lys.reset_all_slot);
        cv_text_on(12, 86 + CAP_IN(S, 13), &AF_S, "G5  CLEAR ALL", lys.reset_all_at ? ink : T_ACCENT, fill);
        cv_text_r(228, 86 + CAP_IN(S, 13), &AF_S, lys.reset_all_at ? info : "HOLD 1s", ink, fill);
    }
    for (p = 0; drum && p < NLANE; p++) {               /* the lanes (black keys 1..8): 8 cells of 18 px, 2 px apart */
        int32_t x = 6 + 20 * (int32_t)p;
        uint16_t ink, fill = lc_box(x, 100, 18, 18, lc_fill(p == ui.lane % NLANE ? LS_SEL : LS_OFF, &ink));
        GFX_HOOK_ALIGN(x, 100, x + 18, 118, AL_HV, "tools lane cell name centred");
        cv_text_in(x, 100 + CAP_IN(S, 18), 18, &AF_S, drum_lane_abbr(TSEL, p), ink, fill);
    }
    for (p = TL_DOUBLE; !eu.on && p < TL_N; p++) {     /* ((1.3) D5 DOUBLE (x2), E5 HALF (/2) at the row's end, */
        int32_t x = 168 + 35 * (int32_t)(p - TL_DOUBLE);   /* 31 px each: the lanes' row has no room for their names) */
        uint16_t ink, fill = lc_box(x, 100, 31, 18, lc_fill(st, &ink));
        n[0] = W_NOTE[p];
        GFX_HOOK_ALIGN(0, 100, 0, 118, AL_V, "tools resize cell on its middle");
        cv_text_on(x + 5, 100 + CAP_IN(S, 18), &AF_S, n, fill == T_RAISE || fill == T_SURF ? T_MID : ink, fill);
        GFX_HOOK_ALIGN(0, 100, 0, 118, AL_V, "tools resize cell on its middle");
        cv_text_r(x + 26, 100 + CAP_IN(S, 18), &AF_S, p == TL_DOUBLE ? "x2" : "/2", ink, fill);
    }
}

/* REC: a row a key (26 px, 4 apart): the key's cell (its note, icon and name), then CLEAR's track and what it holds,
 * or the setting's three values, the one now with the selection's fill (as MENU > AUDIO steps them) */
#define LR_KW 78                                         /* the key's cell; the values 46 px each: x 6 .. 234 */
#define LR_VW 46
#define LR_H 26
static void layer_rec(void)
{
    uint32_t r, v;
    char n[3] = {0, 0, 0}, b[16];
    if (rec_grid_page) {
        cv_text_in(0, 15, 240, &AF_M, "QUANTIZATION", T_THEME, T_BG);
        cv_text_in(0, 46, 240, &AF_S, (rec_prefs_for(song.sel) & 15u) ? "D: DOTTED   T: TRIPLET" :
                   "TRACK: USES PATTERN DIV", T_TEXT, T_BG);
        cv_text_in(0, 68, 240, &AF_S, "OFF: KEEP PLAYED TIMING", T_MID, T_BG);
        cv_text_in(0, 100, 240, &AF_S, "SELECT: REC OPTIONS", T_MID, T_BG);
        return;
    }
    for (r = 0; r < 4u; r++) {
        int32_t y = 4 + (LR_H + 4) * (int32_t)r, x = 6, vx = 6 + LR_KW + 4;
        uint32_t dim = r == 0u && !rec_clearable();
        uint16_t ink, fill = lc_box(x, y, LR_KW, LR_H, lc_fill(dim ? LS_DIM : LS_OFF, &ink));
        n[0] = W_NOTE[r];
        GFX_HOOK_ALIGN(0, y, 0, y + LR_H, AL_V, "rec layer key cell on its middle");
        cv_text_on(x + 5, y + CAP_IN(S, LR_H), &AF_S, n, fill == T_RAISE || fill == T_SURF ? T_MID : ink, fill);
        GFX_HOOK_ALIGN(0, y, 0, y + LR_H, AL_V, "rec layer key cell on its middle");
        if (text_w(&AF_S, r ? RL_LABEL[r - 1u] : "CLEAR") <= LR_KW - 5 - 30)   /* (CRYPT's wider letters: no room) */
            cv_icon_in(x + 15, y, 0, LR_H, 12, r ? RL_ICON[r - 1u] : ICON_CLEAR, ink, fill);
        GFX_HOOK_ALIGN(0, y, 0, y + LR_H, AL_V, "rec layer key cell on its middle");
        cv_text_r(x + LR_KW - 5, y + CAP_IN(S, LR_H), &AF_S, r ? RL_LABEL[r - 1u] : "CLEAR", ink, fill);
        if (r == 0u) {                                   /* "T2 SEQUENCE" / "T2 EMPTY" (a song playing: why not) */
            int32_t w = 234 - vx;
            b[0] = 'T';
            b[1] = (char)('1' + song.sel);
            str_cpy(b + 2, rec_clearable() ? " SEQUENCE" : " EMPTY", sizeof b - 2);
            fill = lc_box(vx, y, w, LR_H, lc_fill(dim ? LS_DIM : LS_OFF, &ink));
            GFX_HOOK_ALIGN(vx, y, vx + w, y + LR_H, AL_HV, "rec layer clear cell centred");
            cv_text_in(vx, y + CAP_IN(S, LR_H), w, &AF_S, b, ink, fill);
            continue;
        }
        for (v = 0; v < 3u; v++, vx += LR_VW + 4) {
            fill = lc_box(vx, y, LR_VW, LR_H, lc_fill(v == rp_get(RL_F[r - 1u]) ? LS_SEL : LS_OFF, &ink));
            GFX_HOOK_ALIGN(vx, y, vx + LR_VW, y + LR_H, AL_HV, "rec layer value cell centred");
            cv_text_in(vx, y + CAP_IN(S, LR_H), LR_VW, &AF_S, rl_name(r - 1u, v), ink, fill);
        }
    }
}

static void layer_cards(uint32_t l)
{
    char val[12];
    const char *unit;
    uint32_t c;
    if (l == LAYER_FX) {                                /* FILTER CRUSH THROW DEPTH */
        int32_t m = perf_k[0];
        fmt_int(val, m < 0 ? -m : m);
        draw_column(0, "FILTER", m ? val : "OFF", m < 0 ? "LP" : m > 0 ? "HP" : "", m ? VR_LIVE : VR_DIM, (m + 100) * 5,
                    ICON_CUTOFF);
        fmt_int(val, perf_k[1]);
        draw_column(1, "CRUSH", perf_k[1] ? val : "OFF", perf_k[1] ? "%" : "", perf_k[1] ? VR_LIVE : VR_DIM,
                    perf_k[1] * 10, ICON_BITS);
        fmt_int(val, perf_k[2]);
        draw_column(2, "THROW", perf_k[2] ? val : "OFF", perf_k[2] ? "%" : "", perf_k[2] ? VR_LIVE : VR_DIM,
                    perf_k[2] * 10, ICON_DELAY);
        if (perf_harm_on()) {                           /* OCT UP / DN playing: KNOB 4 is its shimmer (SHIMR) */
            fmt_int(val, perf_k[3]);
            draw_column(3, "SHIMR", perf_k[3] ? val : "OFF", perf_k[3] ? "%" : "", perf_k[3] ? VR_LIVE : VR_DIM,
                        perf_k[3] * 10, ICON_FEEDBACK);
        } else {
            fmt_int(val, 100 - perf_k[3]);
            draw_column(3, "DEPTH", val, "%", VR_LIVE, (100 - perf_k[3]) * 10, ICON_MIX);
        }
    } else if (l == LAYER_GLO) {                        /* the four tracks' LEVEL (the track's icon; muted: dim; */
        for (c = 0; c < 4u; c++) {                      /* (Skiff phase 6) the four shown, ui.c trk_top) */
            uint32_t k = trk_top() + c;
            param_format(&TP[P_LEVEL], trk[k].p[P_LEVEL], val, &unit);
            draw_column(c, "LEVEL", val, unit, glo_sounding(k) ? VR_LIVE : VR_DIM, RATIO(&TP[P_LEVEL], trk[k].p[P_LEVEL]),
                        trk_icon(k, 1));
        }
    } else if (l == LAYER_EDIT) {
        engine_columns();
    } else if (l == LAYER_PERF) {                       /* (Skiff) the throws as the B mockup shows them: how far */
        static const char *const TL[] = {0, "REVERB", "DELAY", "VOLUME"};
        for (c = 0; c < NELEM(TL); c++) {
            uint32_t id = perform_param_id(TSEL, c);
            const param_desc_t *d = track_desc(TSEL, id);
            int16_t v = TSEL->p[id];
            param_format(d, v, val, &unit);
            draw_column(c, c ? TL[c] : d->label, val, unit, VR_LIVE, RATIO(d, v),
                        c == 3u ? trk_icon(song.sel, 1) : param_icon(d, v));
        }
    } else if (l == LAYER_REC) {                        /* CLICK COUNT-IN LEVEL, (Skiff) FEEL */
        if (rec_grid_page) {
            uint8_t prefs = rec_prefs_for(song.sel);
            int feel = (prefs & REC_TRACK_FEEL) != 0;
            char track[3] = {'T', (char)('1' + song.sel), 0};
            draw_column(0, "GRID", REC_GRID_NAME[prefs & 15u], "", VR_LIVE, -1, ICON_STEPS);
            draw_column(1, "QUANTIZE", feel ? "OFF" : "ON", "", feel ? VR_DIM : VR_LIVE, feel ? 0 : 1000, ICON_TIME);
            draw_column(2, "", "", "", VR_THEME, -1, ICON_NONE);
            draw_column(3, "TRACK", track, "", VR_LIVE, -1, ICON_NONE);
            return;
        }
        for (c = 0; c < 3u; c++) {
            uint32_t v = rp_get(RL_F[c]);
            draw_column(c, RL_LABEL[c], rl_name(c, v), "", c < 2u && !v ? VR_DIM : VR_LIVE, (int32_t)v * 500, RL_ICON[c]);
        }
        draw_column(3, "", "", "", VR_THEME, -1, ICON_NONE);
    } else if (l == LAYER_SEQ && eu.on) {               /* ((1.3) EUCLID: HITS STEPS ROT, the lane (DRUM)) */
        fmt_int(val, eu.hits);
        draw_column(0, "HITS", val, "", VR_LIVE, eu.steps ? eu.hits * 1000 / eu.steps : 0, ICON_PULSE);
        fmt_int(val, eu.steps);
        draw_column(1, "STEPS", val, "", VR_LIVE, eu.steps * 1000 / NSTEP, ICON_STEPS);
        fmt_int(val, eu.rot);
        draw_column(2, "ROTATE", val, "", eu.rot ? VR_LIVE : VR_DIM, eu.steps > 1u ? eu.rot * 1000 / (eu.steps - 1u) : 0,
                    ICON_PHASE);
        if (drum_track(TSEL))
            draw_column(3, "LANE", drum_lane_name(TSEL, ui.lane), "", VR_LIVE, -1, ICON_DRUM);
        else
            draw_column(3, "", "", "", VR_THEME, -1, ICON_NONE);
    } else {                                            /* the page's four (SCL: LY_SCL) */
        uint8_t h = ui.home, pg = ui.page;
        ui.home = 0;
        ui.page = (uint8_t)page_first(LAYERS[l].fam);
        page_over = ly_page(l);
        draw_columns();
        page_over = 0;
        ui.home = h;
        ui.page = pg;
    }
}

static void draw_layer(void)
{
    uint32_t l = ui.layer % LAYER_N, sig = l * 7919u + ux.gen * 977u, rs;
    layer_cards(l);
    if (l == LAYER_FX)
        sig += (perf_kill ? 0u : perf_held | perf_latched) * 31u + perf_latch_on * 11u + perf_act * 131u + perf_avail() * 7u + (uint32_t)perf_harm_on() * 3u +
               glo_mutes() * 7717u + perf_cut * 2803u;   /* ((2.0, D 1A) the black keys: GLO's MUTE) */
    else if (l == LAYER_GLO)
        sig += perf_solo * 31u + (uint32_t)song.g[G_CLOCK] * 5u + (uint32_t)seq_fill * 104729u +
               glo_mutes() * 131u + trk_top() * 1299709u + (perf_cut | perf_held >> PF_M1) * 2803u;   /* ((2.0) holds) */
    else if (l == LAYER_SCL)
        sig += (uint32_t)TSEL->p[P_SCALE] * 31u;
    else if (l == LAYER_REC)
        sig += (uint32_t)ui_rec_prefs * 31u + (uint32_t)rec_clearable() * 5u + song.sel * 977u +
               rec_grid_page * (7919u + rec_prefs_for(song.sel) * 131u);
    else if (l == LAYER_PERF)                           /* (Skiff) */
        sig += bank_gen() * 31u + launch_mode() * 5u + song.sel * 977u + perf_sig() * 131u +
               perf_scenes() * 1299709u + (scene_now() + 1u) * 15485863u + (uint32_t)stage_pending() * 32452843u;
    else if (l == LAYER_SEQ)
        sig += (uint32_t)drum_track(TSEL) * 31u + ui.lane * 5u + (uint32_t)TSEL->p[P_E0] * 131u + song.sel * 977u +
               (lys.reset_all_at ? 617u + lys.reset_all_slot * 29u : 0u) +
               (eu.on ? 7u + eu.hits * 13u + eu.steps * 1031u + eu.rot * 65537u + (uint32_t)TSEL->p[P_SLEN] * 257u : 0u);
    else
        sig += snd_id() * 31u + (uint32_t)preset_favorite() * 5u + up_gen * 101u + song.sel * 977u +
               drum_mute[song.sel] * 2654435761u;   /* ((1.3) the lanes' mutes) */   /* ((Skiff phase 6) its
                                                                                    * track: what it may play) */
    if (ui.force || sig != ui.layer_sig) {
        ui.layer_sig = sig;
        cv_begin(240, H_GRAPH + foot_less(), T_BG);  /* ((Skiff) BLOCKS: the taller panel, a map centred in it; not
                                                      * graph_h: a layer keeps the layout above in MENU > LARGE) */
        cv_bg = T_BG;                                  /* ((Skiff) the cells are the tiles, on the ground, as the
                                                        * mockup's: no panel under them) */
        cv_oy = l == LAYER_PERF ? 0 : graph_pad();
        if (l == LAYER_FX)
            layer_fx();
        else if (l == LAYER_GLO)
            layer_glo();
        else if (l == LAYER_SCL)
            layer_scl();
        else if (l == LAYER_SEQ)
            layer_seq();
        else if (l == LAYER_REC)
            layer_rec();
        else if (l == LAYER_PERF)
            layer_perf();
        else
            layer_edit();
        cv_oy = 0;
        cv_blit(0, Y_GRAPH);
    }
    rs = l == LAYER_FX && !perf_latch_on && mom_held(1) ? 1u + ret_mode() : 0u;
    if (ui.force || rs != lys.ret) {                    /* the footer: what the keys, knobs and buttons do */
        lys.ret = (uint8_t)rs;
        cv_begin(240, foot_h(), T_BG);
        if (rs) {                                       /* ((2.0) FX's macros, a throw out: LET GO .., OCT+: KEEP) */
            ret_hints(rs, foot_line_y());
        } else if (l == LAYER_PERF) {                   /* (Skiff) as the B mockup: what the keys do, on the names' */
            const char *r = "BLACK: MUTE";
            cv_free_hint(8, foot_line_y(), perf_scenes() ? "[SAVE] + WHITE: STORE" : "WHITE: PATTERN", T_MID, T_BG, 128);
            cv_free_hint(232 - head_hint_w(r, 120), foot_line_y(), r, T_MID, T_BG, 120);
        } else {
            const khint_t *ft = l == LAYER_FX && perf_latch_on ? FX_LATCH_FOOT
                              : l == LAYER_GLO && ui.lock == LAYER_GLO ? GLO_LOCK_FOOT
                              : l == LAYER_SEQ && eu.on ? EU_FOOT : LAYERS[l].foot;
            cv_key_row(8, 232, foot_line_y() - KC_TY, ft, ft[2].act ? 3u : 2u, 7u, T_BG);
        }
        cv_blit(0, foot_y());
        ui.foot_sig = 0;
    }
}

/* (Skiff U5, A1; DESIGN.md: every SHIFT combo is on it) HOME held alone (ui.shift): what every control does on this
 * page, 4 rows of 3 cells under the page's cards, drawn as the layer maps (design/mock_u5.py): this page's own jobs
 * TEXT, the ones the same everywhere MID, SHIFT's combos lit; the footer: LET GO: MENU and the page */
typedef struct {
    const char *key, *job;
    uint8_t kind;                                    /* 0 everywhere, 1 this page's, 2 a SHIFT combo */
} smap_t;
static void shift_map(smap_t *m)
{
    uint32_t g = ui.home ? GR_NONE : cur_page()->graph, sel = !ui.home && (step_page(g) || g == GR_FEEL),
             drum = drum_track(TSEL);
    static const char SND[] = "SOUND";              /* (PRESETS' job on most pages: not this page's own) */
    const sheet_t *sh = sheet_of(g);
    const char *pj = ui.home || g == GR_BROWSE ? SND : sel ? "STEP" : g == GR_STEPS ? "DIR" : g == GR_DELAY || g == GR_FXDLY ? "MODE" :
                     g == GR_INS ? "MIX" :            /* ((2.5) INSERT 2's page before) */
                     g == GR_USER || g == GR_SLOTS ? "SLOT" : g == GR_BANK ? "PATTERN" : g == GR_EVENTS ? "ROW" :
                     g == GR_SONG ? "ENTRY" : sh ? sh->pj : g == GR_SLICES ? "SLICE" : SND;
    static const smap_t ALL[9] = {{"BUTTONS", "PAGES", 0}, {"HOLD ONE", "ITS LAYER", 0}, {"HOLD SAVE", "UNDO", 0},
                                  {"+ SAVE", "TAKE", 2}, {"+ REC", "REC SET", 2}, {"+ BUTTON", "LOCK", 2},
                                  {"+ PRESETS", "RANGE", 2}, {"+ OCT-", "COPY", 2}, {"+ OCT+", "PASTE", 2}};   /* ((2.0)
                                                     * NOTES, FEEL: the selection's, ui_input.c sel_shift, sel_copy) */
    uint32_t i;
    m[0] = (smap_t){"PRESETS", pj, (uint8_t)(pj != SND)};
    if (ui.home && !(ui_prefs & PREF_BPM_LOCK))
        m[1] = (smap_t){"SELECT", "TEMPO", 1};
    else if (ui.home)
        m[1] = (smap_t){"GLO + SELECT", "TEMPO", 0};
    else
        m[1] = (smap_t){"SELECT", sh ? "ROWS" : "PAGE", (uint8_t)(sh != 0)};
    m[2] = (smap_t){"ALGO", "TRACK", 0};
    m[3] = (smap_t){"KEYS", drum ? "DRUMS" : "NOTES", 0};
    m[4] = !ui.home && act_cols() ? (smap_t){"OCT+", "DO IT", 1} : (smap_t){"OCT- +", "OCTAVE", 0};
    m[5] = (smap_t){"PLAY", song.playing ? "STOP" : "START", 0};   /* (what it would do now) */
    if (step_page(g) && !drum) {                     /* ((2.0) NOTES: the keys pick, with REC write) */
        m[3] = (smap_t){"KEYS", "PICK", 1};
        m[4] = (smap_t){"KEYS + REC", "WRITE", 1};
        m[5] = (smap_t){"EDIT + REC", "CLEAR", 1};
    } else if (step_page(g)) {                       /* (the grid: the keys its steps) */
        m[3] = (smap_t){"KEYS", "STEPS", 1};
        m[4] = (smap_t){"EDIT + REC", "CLEAR", 1};
    }
    for (i = 0; i < 6u; i++)
        m[6 + i] = ALL[i < 3u && sel ? 6u + i : i];
    if (sel)                                         /* ((2.0) there SHIFT + EDIT deletes, ui_input.c sel_delete) */
        m[11] = (smap_t){"+ EDIT", "DELETE", 2};
    if (g == GR_SONG) {                              /* ((2.5) SONG: + PLAY the song from the entry, + EDIT empties it) */
        m[6] = (smap_t){"+ PLAY", "FROM HERE", 2};
        m[11] = (smap_t){"+ EDIT", "CLEAR", 2};
    }
#if BOARD_SMK37
    {   /* (SMK-37 Pro, SMK37_UX.md question 10) the controls by the SMK's printed names: K2 SELECT, K3 ALGORITHM, K4
         * PRESETS, PARA EDIT, BT SAVE, SEQ REC REC, KNOB BANK SHIFT. PLAY stays PLAY: the SMK prints a play sign there,
         * which the screen's fonts don't have */
        static const char *const FM[] = {"PRESETS", "SELECT", "GLO + SELECT", "ALGO", "EDIT + REC", "HOLD SAVE",
                                         "+ SAVE", "+ REC", "KEYS + REC", "+ PRESETS", "+ EDIT"};
        static const char *const SMK[] = {"K4", "K2", "GLOBE + K2", "K3", "PARA + SEQ REC", "HOLD BT",
                                          "+ BT", "+ SEQ REC", "KEYS + SEQ REC", "+ K4", "+ PARA"};
        uint32_t j;
        for (i = 0; i < 12u; i++)
            for (j = 0; j < NELEM(FM); j++)
                if (str_eq(m[i].key, FM[j]))
                    m[i].key = SMK[j];
    }
#endif
}
static void draw_shift(void)
{
    smap_t m[12];
    char ti[20];
    uint32_t i, sig = 0x5A1F7u + ui.page * 7919u + ui.home * 31u + song.sel * 977u + ux.gen * 13u + ui_prefs * 3u + song.playing * 104729u;
    if (!ui.force && sig == ui.layer_sig)
        return;
    ui.layer_sig = sig;
    shift_map(m);
    cv_begin(240, H_GRAPH + foot_less(), T_BG);
    cv_bg = T_BG;
    for (i = 0; i < 12u; i++) {
        int32_t x = 4 + 78 * (int32_t)(i % 3u), y = 2 + 34 * (int32_t)(i / 3u);
        uint16_t bg = m[i].kind == 2u ? T_TEXT : T_SURF;
        if (ux.crypt && m[i].kind != 2u) {               /* (Skiff C3) CRYPT: stone, framed */
            cr_stone(x, y, 74, 31, bg);
            cv_frame(x, y, 74, 31, T_LINE);
        } else
            cv_rect(x, y, 74, 31, bg);
        cv_text_on(x + 5, y + 12 - AF_S.asc, &AF_S, m[i].key, m[i].kind == 2u ? T_INK : T_MID, bg);
        cv_text_fit(x + 5, y + 27 - AF_M.asc, &AF_M, m[i].job, m[i].kind == 2u ? T_INK : m[i].kind ? T_TEXT : T_MID, bg, 66);
    }
    cv_blit(0, Y_GRAPH);
    cv_begin(240, foot_h(), T_BG);
#if BOARD_SMK37
    if (!((fm1_in.buttons >> panel.btn[B_HOME]) & 1u))   /* (KNOB BANK's SHIFT: let go, nothing) */
        cv_free_hint(8, foot_line_y(), "SHIFT", T_MID, T_BG, 110);
    else
#endif
    cv_free_hint(8, foot_line_y(), "LET GO: MENU", T_MID, T_BG, 110);
    if (ui.home)
        str_cpy(ti, "HOME", sizeof ti);
    else
        str_cpy(ti, page_head(), sizeof ti);
    cv_text_r(232, foot_line_y(), &AF_S, ti, T_MID, T_BG);
    cv_blit(0, foot_y());
    ui.foot_sig = 0;
}
