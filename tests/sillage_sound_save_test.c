/* Local Sillage V3 candidate: real UI input, drawing and preset storage paths. */
#define UI_TEST_NO_MAIN 1
static unsigned int audio_peak_us;
#include "ui_test.c"

static uint32_t save_sound_page_id(void)
{
    for (uint32_t i = 0; i < NPAGES; i++)
        if (PAGES[i].fam == FAM_EDIT && PAGES[i].graph == GR_USER) return i;
    return NPAGES;
}

static void sound_save_page(void)
{
    ui.home = 0;
    ui.design_track = (uint8_t)(song.sel + 1u);
    ui.design_engine = TSEL->eng_req; ui.design_color=1;
    ui.page = (uint8_t)save_sound_page_id();
    page_entered();
    frame();
    press(B_OCTUP);
}

int main(void)
{
    int bad = 0;
    char label[16];
    uint32_t page;
    ui_power_on();
    set_engine_of(TSEL, 0);
    apply_preset_to(TSEL, 0);
    frame();
    bad += check("sound star: factory preset starts clean", !ui.sound_dirty);
    int16_t original = TSEL->p[P_E0];
    TSEL->p[P_E0] = original == 0 ? 1 : original - 1;
    frame();
    sound_name(TSEL, label);
    bad += check("sound star: edited parameter marks name", ui.sound_dirty && label[0] == '*');
    TSEL->p[P_E0] = original;
    frame();
    bad += check("sound star: exact return to preset clears marker", !ui.sound_dirty);
    TSEL->p[P_LEVEL]--;
    TSEL->p[P_PAN] = 17;
    TSEL->p[P_MUTE] = 1;
    TSEL->p[P_GROOVE_HUMAN] = 21;
    TSEL->step[0].time = ST_NOTE;
    TSEL->step[0].note[0] = 60;
    TSEL->step[0].n = 1;
    frame();
    bad += check("sound star: mix, mute, groove and notes are not preset edits", !ui.sound_dirty);
    TSEL->p[P_E0] = original == 0 ? 1 : original - 1;
    frame();
    sel_set(1);
    set_engine_of(TSEL, 0);
    frame();
    bad += check("sound star: another clean track stays clean", !ui.sound_dirty);
    sel_set(0);frame();
    bad += check("sound star: edited track retains its marker", ui.sound_dirty);

    page = save_sound_page_id();
    bad += check("SAVE SOUND belongs only to sound creation, not normal EDIT", page < NPAGES && page == NPAGES - 1u && !page_visible(page));
    /* Reproduce the exact reported route: choose an engine in PRESET INIT,
     * confirm it, edit a bare sound, then cycle with both physical controls. */
    for (uint32_t method = 0; method < 2u; method++) {
        go_home();
        ui.home=0;ui.page=(uint8_t)(page-1u);page_entered();
        ui.init_engine=1u;press(B_EDIT);turn(EN_K1,1);
        int found=0;
        for(uint32_t n=0;n<NPAGES+12u;n++) {
            if(ui.page==page){found=1;break;}
            if(method)press(B_EDIT);else turn(EN_SELECT,1);
        }
        bad += check(method?"INIT -> edit bare ANALOG -> EDIT reaches SAVE SOUND":"INIT -> edit bare ANALOG -> SELECT reaches SAVE SOUND",found);
        bad += check("creation is saturated blue, bound to track and engine, and skips INIT", sound_design_active() && T_BG==0x11EFu && !page_visible(page-1u));
    }
    uint32_t last=0;
    for(uint32_t i=0;i<page;i++)if(PAGES[i].fam==FAM_EDIT&&page_visible(i))last=i;
    ui.home=0;ui.page=(uint8_t)last;page_entered();
    turn(EN_SELECT,1);
    bad += check("SELECT reaches SAVE SOUND after the engine's editing pages", ui.page == page);
    press(B_EDIT);
    bad += check("EDIT cycles from SAVE SOUND back to engine pages", cur_page()->fam == FAM_EDIT && ui.page != page);
    sound_save_page();
    bad += check("SAVE SOUND: OCT+ opens the existing USER slot page with SAVE selected", cur_page()->fam==FAM_SAVE && cur_page()->graph==GR_USER && ui.act==4u);
    ui.uslot = 3;
    turn(EN_K1, 1);
    bad += check("SAVE SOUND: K1 changes destination without loading a sound", ui.uslot == 4u && ui.sound_dirty);
    uint32_t before_engine=TSEL->eng_req;
    int16_t before_note=TSEL->step[0].note[0], before_level=TSEL->p[P_LEVEL];
    press(B_OCTUP);
    bad += check("SAVE SOUND: OCT+ opens preset naming before any write", name_on() && nm.kind == NK_USER_SAVE && !up_used(4));
    str_cpy(nm.s,"MY BASS",sizeof nm.s);nm.len=7;nm.cur=7;nm.key=0;
    press(B_OCTUP);
    up_name(4,label);
    bad += check("SAVE SOUND: final OCT+ saves chosen name and slot, clears star", up_used(4) && str_eq(label,"MY BASS") && !ui.sound_dirty && !name_on());
    bad += check("saving ends creation and restores the usual background", !ui.design_track && !design_tinted && T_BG!=0x11EFu);
    bad += check("SAVE SOUND: save retains track, sequence and level", TSEL->eng_req == before_engine && TSEL->step[0].note[0] == before_note && TSEL->p[P_LEVEL] == before_level);
    TSEL->p[P_E0] = original;
    frame();
    bad += check("sound star: editing a saved user preset marks it again", ui.sound_dirty);
    sound_save_page();ui.uslot=4;
    press(B_OCTUP);
    bad += check("SAVE SOUND: used destination requests overwrite first", ui.confirm == CF_OVR_USER && !name_on());
    press(B_OCTDN);
    bad += check("SAVE SOUND: cancel overwrite preserves saved sound and dirty edit", !ui.confirm && !name_on() && ui.sound_dirty);
    turn(EN_K4,1);press(B_OCTUP);press(B_OCTUP);
    bad += check("SAVE SOUND: confirming overwrite opens naming", name_on() && nm.kind == NK_USER_SAVE);
    press(B_OCTDN);
    bad += check("SAVE SOUND: cancelling name writes nothing", !name_on() && ui.sound_dirty);
    sound_save_page();song.playing=1;
    press(B_OCTUP);
    bad += check("SAVE SOUND: playing cannot write flash", !name_on() && !ui.confirm && str_eq(ui.msg,"STOP TO SAVE"));
    song.playing=0;

    set_engine_of(TSEL, ENGI_FM6);apply_preset_to(TSEL,0);frame();
    uint8_t patch[FP_SIZE+1u];memcpy(patch,fm6_patch[song.sel],sizeof patch);
    patch[0]=patch[0]?patch[0]-1u:1u;fm6_set_patch(song.sel,patch);fm6_adopt(song.sel);frame();
    bad += check("sound star: FM6 operator changes are detected", ui.sound_dirty);
    sound_save_page();ui.uslot=5;press(B_OCTUP);press(B_OCTUP);
    bad += check("SAVE SOUND: FM6 operator patch saved through existing path", up_used(5) && !ui.sound_dirty);

    ui.home=0;ui.design_track=(uint8_t)(song.sel+1u);ui.design_engine=TSEL->eng_req;ui.design_color=1;
    ui.page=(uint8_t)page_vis_first(FAM_EDIT);page_entered();frame();
    press(B_ENV);
    bad += check("ENV restores normal colour without losing the sound journey", sound_design_active() && T_BG!=0x11EFu && track_context());
    press(B_LFO);
    bad += check("LFO restores normal colour without losing the sound journey", sound_design_active() && T_BG!=0x11EFu && track_context());
    press(B_FX);
    bad += check("FX restores normal colour", sound_design_active() && T_BG!=0x11EFu);
    press(B_EDIT);
    bad += check("returning to EDIT restores its blue identity", sound_design_active() && T_BG==0x11EFu);
    hold(B_FX);
    bad += check("FX HOLD restores normal colour even over an EDIT page", ui.layer==LAYER_FX && T_BG!=0x11EFu);
    press(B_FX);
    press(B_HOME);
    bad += check("HOME exits creation and restores the normal theme", !ui.design_track && !design_tinted && T_BG!=0x11EFu);
    ui.home=0;ui.design_track=(uint8_t)(song.sel+1u);ui.design_engine=TSEL->eng_req;ui.design_color=1;
    ui.page=(uint8_t)page_vis_first(FAM_EDIT);page_entered();frame();
    sel_set(1);frame();
    bad += check("changing track cannot leak the creation mode or tint", !ui.design_track && !design_tinted && T_BG!=0x11EFu);
    settings.palette=UI_SILLAGE_LIGHT_INDEX;theme_apply();
    ui.home=0;ui.design_track=(uint8_t)(song.sel+1u);ui.design_engine=TSEL->eng_req;ui.design_color=1;
    ui.page=(uint8_t)page_vis_first(FAM_EDIT);page_entered();frame();
    bad += check("creation remains readable when the saved theme is light", T_BG==0x11EFu && T_TEXT==UI_PALETTES[UI_SKIFF_INDEX].text && settings.palette==UI_SILLAGE_LIGHT_INDEX);
    press(B_HOME);
    bad += check("leaving creation restores the original light theme", T_BG==UI_PALETTES[UI_SILLAGE_LIGHT_INDEX].bg && settings.palette==UI_SILLAGE_LIGHT_INDEX);

    bad += test_name(); /* existing SAVE > USER naming/overwrite/error behaviours */
    printf(bad ? "sound-save: %d FAILED\n" : "sound-save: all passed\n",bad);
    return bad?1:0;
}
