/* Test real playback overlay, UI controls, phrase integrity and scale bounds. */
#define UI_TEST_NO_MAIN 1
static unsigned int audio_peak_us;
#include "ui_test.c"

static void phrase(track_t *t)
{
    memset(t->step, 0, sizeof t->step);
    t->p[P_SLEN] = 16; t->p[P_SDIV] = 2; t->p[P_SGATE] = 64;
    t->p[P_SCALE] = 2; t->p[P_ROOT] = 2;
    const uint8_t pos[] = {0,4,6,9,10,12};
    const uint8_t note[] = {62,65,69,67,65,62};
    for (uint32_t i = 0; i < 6; i++) {
        step_t *s = &t->step[pos[i]];
        s->time = ST_NOTE; s->n = 1; s->note[0] = note[i]; s->vel = 96;
    }
    t->seq_active = 1;
}
int main(void)
{
    int bad = 0;
    ui_power_on(); set_engine_of(TSEL, 0); phrase(TSEL);
    step_t backup[NSTEP], tmp; memcpy(backup, TSEL->step, sizeof backup);
    uint64_t before = 0;
    for (uint32_t amount = 0; amount <= 100; amount++) {
        uint64_t mask = 0;
        for (uint32_t i = 0; i < 16; i++) if (evolve_note(TSEL, i, amount, &tmp)) {
            mask |= (uint64_t)1 << i;
            if (backup[i].n || !((scale_mask(TSEL) >> ((tmp.note[0] + 120 - TSEL->p[P_ROOT]) % 12)) & 1u)) bad++;
            step_t again; if (!evolve_note(TSEL, i, amount, &again) || memcmp(&tmp,&again,sizeof tmp)) bad++;
        }
        if ((before & mask) != before) bad++;
        before = mask;
    }
    bad += check("increasing EVOLVE only adds stable in-scale notes on empty steps", !bad && __builtin_popcountll(before)==10);
    bad += check("original phrase never rewritten", !memcmp(backup,TSEL->step,sizeof backup));
    evolve_request(0,100);
    bad += check("stopped request immediately active, 16-byte state", evolve_live[0]==100 && sizeof evolve_live+sizeof evolve_want==16);
    song.playing = 1; evolve_request(0,0);
    TSEL->seq_cnt=2; TSEL->seq_idx=1; seq_enter(TSEL,div_samples(2),16);
    bad += check("reset waits through current loop", evolve_live[0]==100 && evolve_want[0]==0);
    TSEL->seq_cnt=1; TSEL->seq_idx=0; seq_enter(TSEL,div_samples(2),16);
    bad += check("reset applies at first step of next loop", !evolve_live[0]);
    /* Actual notes through seq_fire, not just the generator helper. */
    evolve_live[0]=100;
    uint32_t idx=1;
    while(idx<16 && !evolve_note(TSEL,idx,100,&tmp)) idx++;
    seq_fire(TSEL,idx,div_samples(2),0);
    bad += check("generated note reaches sequencer voice path", TSEL->seq_n==1 && TSEL->seq_notes[0]==tmp.note[0]);
    evolve_live[0]=0; seq_fire(TSEL,idx,div_samples(2),0);
    bad += check("zero/reset restores silent gap through real sequencer", TSEL->seq_n==0);
    TSEL->step[1].time=ST_TIE;
    bad += check("tie and its continuation protected", !evolve_note(TSEL,1,100,&tmp) && !evolve_note(TSEL,2,100,&tmp));
    memcpy(TSEL->step,backup,sizeof backup);
    TSEL->step[1].probability=101;
    bad += check("explicitly muted rest protected", !evolve_note(TSEL,1,100,&tmp));
    memset(TSEL->step,0,sizeof TSEL->step);
    bad += check("empty pattern stays silent", !evolve_note(TSEL,1,100,&tmp));
    phrase(TSEL); song.playing=0;
    for(uint32_t i=0;i<NENGINES;i++) if(drum_engine(i)) {
        TSEL->eng_req=(uint8_t)i;
        bad += check("drum engine excluded", !evolve_allowed(TSEL) && !evolve_note(TSEL,1,100,&tmp));
    }
    set_engine_of(TSEL,0);
    for(uint32_t i=0;i<NPAGES;i++) if(PAGES[i].graph==GR_EVOLVE) {ui.page=(uint8_t)i;ui.home=0;break;}
    page_entered(); frame();
    bad += check("EVOLVE follows FEEL in SEQ", ui.page && PAGES[ui.page-1].graph==GR_FEEL);
    turn(EN_K1,55); bad += check("physical K1 sets track dose", evolve_want[0]==55);
    press(B_OCTDN); bad += check("OCT- resets selected track without changing octave", !evolve_want[0]);
    evolve_request(0,30); evolve_request(7,60);
    press(B_OCTUP); bad += check("OCT+ resets every track", !evolve_want[0]&&!evolve_want[7]);
    for(uint32_t i=0;i<7;i++) turn(EN_ALGO,1);
    set_engine_of(TSEL,0); turn(EN_K1,35);
    bad += check("T8 selectable and independently adjustable", song.sel==7 && evolve_want[7]==35);
    bad += check("track selection retains T1 original dose", !evolve_want[0]);
    int boundaries=1;
    phrase(TSEL);
    for(uint32_t scale=0;scale<sizeof SCALE_MASK/sizeof SCALE_MASK[0];scale++)
        for(uint32_t root=0;root<12;root++)
            for(uint32_t edge=0;edge<=127;edge+=127) {
                TSEL->p[P_SCALE]=(int16_t)scale; TSEL->p[P_ROOT]=(int16_t)root;
                TSEL->step[0].note[0]=(uint8_t)edge;
                if(evolve_note(TSEL,1,100,&tmp) &&
                   (tmp.note[0]>127 || !((scale_mask(TSEL)>>((tmp.note[0]+120-root)%12))&1u))) boundaries=0;
            }
    bad += check("all scales and roots remain valid at both MIDI pitch limits",boundaries);
    evolve_request(0,70); evolve_request(7,80);
    project_new();
    bad += check("new project clears transient EVOLVE",!evolve_want[0]&&!evolve_live[0]&&!evolve_want[7]&&!evolve_live[7]);
    sel_set(0); set_engine_of(TSEL,0); phrase(TSEL); song.playing=0; evolve_clear();
    uint8_t base_note[64]={0}, old_oct[64]={0};
    for(uint32_t i=0;i<16;i++) if(evolve_note(TSEL,i,100,&tmp)) base_note[i]=tmp.note[0];
    evolve_octave[0]=1; evolve_request(0,30);
    int stacked=1, first_count=0, second_count=0, low_count=0;
    for(uint32_t i=0;i<16;i++) if(evolve_note_at(TSEL,i,evolve_live[0],&tmp,1)) {
        first_count++;old_oct[i]=1;if(tmp.note[0]!=base_note[i]+12) stacked=0;
    }
    evolve_octave[0]=0; evolve_request(0,65);
    for(uint32_t i=0;i<16;i++) if(evolve_note_at(TSEL,i,evolve_live[0],&tmp,1)) {
        if(old_oct[i]) {if(tmp.note[0]!=base_note[i]+12)stacked=0;}
        else {second_count++;old_oct[i]=2;if(tmp.note[0]!=base_note[i])stacked=0;}
    }
    evolve_octave[0]=2; evolve_request(0,100);
    for(uint32_t i=0;i<16;i++) if(evolve_note_at(TSEL,i,evolve_live[0],&tmp,1)) {
        int off=old_oct[i]==1?12:old_oct[i]==2?0:-12;
        if(!old_oct[i])low_count++;
        if(tmp.note[0]!=base_note[i]+off)stacked=0;
    }
    bad+=check("K3 octave stages retain +1, then 0, then add -1",stacked&&first_count&&second_count&&low_count);
    song.playing=1;
    evolve_request(0,20); evolve_octave[0]=0; evolve_request(0,100);
    int unchanged=1;
    for(uint32_t i=0;i<16;i++) if(evolve_note_at(TSEL,i,100,&tmp,1)) {
        int off=old_oct[i]==1?12:old_oct[i]==2?0:-12;
        if(tmp.note[0]!=base_note[i]+off)unchanged=0;
    }
    bad+=check("pending octave reassignment cannot change current-loop pitches",unchanged);
    /* Same amount after removing/re-adding candidates must still commit maps. */
    TSEL->seq_cnt=1;TSEL->seq_idx=0;seq_enter(TSEL,div_samples(2),16);
    int reassigned=1;
    for(uint32_t i=0;i<16;i++) if(evolve_note_at(TSEL,i,100,&tmp,1)) {
        uint32_t rank=(i*37u+17u)&63u;
        if(rank>=evolve_threshold(20) && tmp.note[0]!=base_note[i])reassigned=0;
    }
    bad+=check("same final density commits octave changes at loop boundary",reassigned);
    evolve_length(0,-30); /* 20% */
    TSEL->seq_cnt=2;TSEL->seq_idx=1;seq_enter(TSEL,div_samples(2),16);
    bad+=check("new length waits until next loop",!evolve_len_live[0]);
    TSEL->seq_cnt=1;TSEL->seq_idx=0;seq_enter(TSEL,div_samples(2),16);
    seq_fire(TSEL,1,div_samples(2),0);
    bad+=check("added note uses K4 gate length",TSEL->seq_off==step_samples(TSEL,div_samples(2),1)/100u*20u);
    seq_fire(TSEL,0,div_samples(2),0);
    bad+=check("original note retains its original gate",TSEL->seq_off==div_samples(2)*(uint32_t)TSEL->p[P_SGATE]/128u);
    song.playing=0;evolve_request(0,0);evolve_octave[0]=1;evolve_request(0,100);
    bad+=check("reset and rebuild discards all previous octave assignments",evolve_note_at(TSEL,1,100,&tmp,1)&&tmp.note[0]==base_note[1]+12);
    for(uint32_t i=0;i<NPAGES;i++)if(PAGES[i].graph==GR_EVOLVE){ui.page=(uint8_t)i;ui.home=0;break;}
    frame();turn(EN_K3,1);turn(EN_K4,15);
    bad+=check("physical K3 selects -1 and K4 adjusts added-note length",evolve_octave[0]==2&&evolve_len_want[0]==35);
    if(!bad) puts("EVOLVE: all passed");
    return bad?1:0;
}
