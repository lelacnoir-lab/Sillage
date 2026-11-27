#define UI_TEST_NO_MAIN 1
static unsigned int audio_peak_us;
#include "ui_test.c"
static void reset_fx(void)
{
 memset(&pf,0,sizeof pf);pf.src=pf.next=PF_N;pf.lc=PF_TOP;pf.gg=32768;
 for(uint32_t k=0;k<NTRK;k++)pf.mg[k]=32768;
 memset((void*)perf_k,0,sizeof perf_k);perf_held=perf_act=perf_latched=0;
 perf_kill=perf_cut=perf_solo=perf_latch_on=0;sl_lent=0;perf_quant=0;
 song.playing=1;song.g[G_BPM]=120;
}
static void process_fx(uint32_t at,int32_t *l,int32_t *r,int32_t *send)
{
 for(uint32_t i=0;i<32;i++){l[i]=((at+i)&1u)?-9000:12000;r[i]=-l[i];send[i]=0;}
 if(at==256){l[0]=60000;r[0]=-60000;} /* kick transient precisely on the target boundary */
 perf_begin(32);perf_dub(l,r,send,32);perf_filter_dry(l,r,32);perf_block(l,r,32);
}
int main(void)
{
 int bad=0;ui_power_on();uint32_t cases=0;
 for(uint32_t grid=1;grid<6;grid++)for(uint32_t effect=0;effect<PF_M1;effect++){
  reset_fx();uint32_t bit=PF_BIT(effect);perf_press(effect,1);
  int32_t l[32],r[32],sd[32];
  for(uint32_t i=0;i<44100;i+=32)process_fx(i,l,r,sd);
  if(effect==PF_LPF){pf.lc=PF_LOW;pf.la=32768;pf.lt=16*(beat_samples()>>2);}
  if(effect==PF_HPF){pf.hc=PF_LOW;pf.ha=32768;pf.ht=16*(beat_samples()>>2);}
  if(effect==PF_GATE)pf.gg=0;
  if(effect==PF_DUB)pf.dd=32768;
  perf_quant=(uint8_t)grid;uint32_t span=(beat_samples()>>2)*FXQ_STEPS[grid];pf.ph=span-256;
  perf_press(effect,0);
  for(uint32_t at=0;at<=320;at+=32){
   process_fx(at,l,r,sd);
   if(at<256 && !(perf_act&bit)){bad++;puts("logical effect ended before its grid");break;}
   if(at==256){
    if(perf_act&bit){bad++;printf("missed deadline grid %u effect %u\n",grid,effect);}
    if(l[0]!=60000||r[0]!=-60000){bad++;printf("kick still processed at deadline grid %u effect %u: %d\n",grid,effect,l[0]);}
    if(effect==PF_DUB&&sd[0]!=0){bad++;puts("DUB send still active on target kick");}
   }
  }
  cases++;
 }
 bad+=check("70 effect/grid combinations: target kick is fully dry, logical timing intact",!bad&&cases==70);
 reset_fx();perf_quant=2;pf.act=perf_act=PF_BIT(PF_LPF);pf.busy=1;pf.la=32768;pf.lc=PF_LOW;
 uint32_t boundary=(beat_samples()>>2)*4;pf.ph=boundary-256;perf_held=PF_BIT(PF_HPF);
 int32_t ml[32],mr[32],ms[32];int switches_ok=1;
 for(uint32_t t=0;t<=256;t+=32){process_fx(t,ml,mr,ms);if(t<256&&pf.ha)switches_ok=0;}
 bad+=check("simultaneous filter change: old fade ends before new FX starts on grid",switches_ok&&!pf.la&&pf.ha&&(perf_act&PF_BIT(PF_HPF))&&!(perf_act&PF_BIT(PF_LPF)));
 reset_fx();perf_quant=2;pf.act=perf_act=PF_BIT(PF_LPF);pf.busy=1;pf.la=32768;pf.lc=PF_LOW;
 uint32_t span=(beat_samples()>>2)*4;pf.ph=span-16;
 perf_begin(32);
 bad+=check("late request keeps the current boundary instead of skipping a grid",!(perf_act&PF_BIT(PF_LPF))&&pf.split==16);
 if(!bad)puts("fx-deadline: all passed");return bad?1:0;
}
