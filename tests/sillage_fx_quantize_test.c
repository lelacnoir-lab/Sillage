/* Real FX state machine: quantized starts AND stops, sample split and UI. */
#define UI_TEST_NO_MAIN 1
static unsigned int audio_peak_us;
#include "ui_test.c"
static void fx_reset(void)
{
    memset(&pf,0,sizeof pf);pf.src=pf.next=PF_N;pf.lc=PF_TOP;pf.gg=32768;
    for(uint32_t k=0;k<NTRK;k++)pf.mg[k]=32768;
    perf_held=perf_latched=perf_act=perf_cut=perf_solo=perf_kill=perf_latch_on=0;
    memset((void*)perf_k,0,sizeof perf_k);song.playing=1;song.g[G_BPM]=120;
}
int main(void)
{
    int bad=0;ui_power_on();
    uint32_t tested=0;
    int32_t left[128]={0},right[128]={0};
    for(uint32_t grid=1;grid<6;grid++)for(uint32_t effect=0;effect<PF_M1;effect++){
        fx_reset();perf_quant=(uint8_t)grid;
        uint32_t span=(beat_samples()>>2)*FXQ_STEPS[grid],bit=PF_BIT(effect);
        pf.ph=1;perf_press(effect,1);
        int busy=perf_begin(128);
        if((perf_act&bit)||busy){bad++;printf("early start / idle DSP grid %u effect %u\n",grid,effect);}
        pf.ph=span-16;perf_begin(128);
        if(!(perf_act&bit)||(pf.act0&bit)||pf.split!=16){bad++;puts("start split wrong");}
        memset(left,0,sizeof left);memset(right,0,sizeof right);perf_block(left,right,128);
        perf_press(effect,0);pf.ph=span+1;perf_begin(128);
        if(!(perf_act&bit)){bad++;puts("stop before grid");}
        pf.ph=2*span-16;perf_begin(128);
        if((perf_act&bit)||!(pf.act0&bit)||pf.split!=16){bad++;puts("stop split wrong");}
        tested++;
    }
    bad+=check("all 14 effects: starts/stops on five grids, exact split, waiting idle",!bad&&tested==70);
    fx_reset();perf_quant=5;pf.ph=1;perf_press(PF_LPF,1);perf_press(PF_LPF,0);perf_begin(128);
    bad+=check("momentary tap released before grid cancels cleanly",!(perf_act&PF_BIT(PF_LPF)));
    fx_reset();perf_quant=5;perf_latch_on=1;pf.ph=1;perf_press(PF_LPF,1);perf_press(PF_LPF,0);perf_begin(128);
    bad+=check("latched tap remains queued after release",(perf_latched&PF_BIT(PF_LPF))&&!perf_act);
    seq_stop();perf_begin(128);
    bad+=check("STOP cancels start still waiting; no surprise tail effect",!perf_act&&!perf_latched);
    fx_reset();perf_quant=0;pf.ph=1;perf_press(PF_REV,1);perf_begin(128);
    bad+=check("NOW starts immediately, including reverse",(perf_act&PF_BIT(PF_REV))&&pf.split==128);
    perf_press(PF_REV,0);pf.busy=1;perf_begin(128);
    bad+=check("NOW stops immediately",!(perf_act&PF_BIT(PF_REV)));
    fx_reset();perf_quant=5;pf.ph=1;perf_press(PF_M1,1);perf_begin(128);
    bad+=check("black-key track mutes are not delayed",(perf_act&PF_BIT(PF_M1))!=0);
    perf_press(PF_M1,0);pf.busy=1;perf_begin(128);
    bad+=check("track mute release stays immediate",!(perf_act&PF_BIT(PF_M1)));
    fx_reset();song.playing=0;perf_quant=5;pf.ph=1;perf_press(PF_FRZ,1);perf_begin(128);
    bad+=check("stopped transport never waits for a nonexistent grid",(perf_act&PF_BIT(PF_FRZ))!=0);
    perf_kill=1;pf.busy=1;perf_begin(128);
    bad+=check("menu kill bypasses quantization",!perf_act);
    fx_reset();song.playing=0;perf_quant=2;shift_lock(B_FX);turn(EN_K2,3);
    bad+=check("real FX HOLD knob 2 selects 2 BAR without CRUSH",perf_quant==5&&perf_k[1]==0);
    press(B_FX);bad+=check("quantization survives closing FX layer",perf_quant==5);
    if(!bad)puts("fx-quantize: all passed");return bad?1:0;
}
