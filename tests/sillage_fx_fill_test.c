#define UI_TEST_NO_MAIN 1
static unsigned int audio_peak_us;
#include "ui_test.c"
int main(void)
{
 int bad=0;ui_power_on();frame();shift_lock(B_FX);
 bad+=check("physical FX HOLD panel is latched open",ui.layer==LAYER_FX);
 perf_quant=5;perf_latch_on=1;
 fm1_in.notes=1u<<23;keyboard_block();frame();
 bad+=check("E5 Fill is immediate even with 2 BAR and FX LATCH",seq_fill_on()&&!(perf_latched&PF_BIT(PF_VIN)));
 bad+=check("old vinyl effect cannot be selected by E5",perf_key(23)==PF_N);
 step_t s={0};s.cond=TC_FILL;
 int melodic=step_cond_ok(&trk[0],&s,0);s.cond=TC_NFILL;
 bad+=check("existing FILL and NOT FILL work on a melodic track",melodic&&!step_cond_ok(&trk[0],&s,0));
 s.cond=TC_FILL;
 bad+=check("same Fill works on drum tracks",step_cond_ok(&trk[3],&s,0));
 fm1_in.notes=0;keyboard_block();frame();
 bad+=check("key release ends Fill without a latch",!seq_fill_on());
 seq_fill=1;seq_fx_fill=1;fm1_in.notes=1u<<23;keyboard_block();
 fm1_in.notes=0;keyboard_block();
 bad+=check("releasing FX Fill preserves a held GLO Fill",seq_fill_on());
 seq_fill=0;
 bad+=check("both access points released leaves no stuck Fill",!seq_fill_on());
 if(!bad)puts("FX FILL: all passed");return bad?1:0;
}
