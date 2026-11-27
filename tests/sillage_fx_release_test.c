#define UI_TEST_NO_MAIN 1
static unsigned int audio_peak_us;
#include "ui_test.c"
int main(void)
{
 int bad=0;ui_power_on();song.g[G_BPM]=120;song.playing=1;perf_quant=2;
 for(uint32_t kind=0;kind<2;kind++){
  memset(&pf,0,sizeof pf);pf.src=pf.next=PF_N;pf.gg=32768;
  memset((void*)perf_k,0,sizeof perf_k);perf_kill=perf_cut=perf_solo=perf_latch_on=0;
  uint32_t bit=PF_BIT(kind?PF_HPF:PF_LPF),span=(beat_samples()>>2)*4u;
  perf_held=0;perf_latched=0;perf_act=bit;pf.act=bit;pf.busy=1;
  pf.lc=kind?PF_TOP:PF_LOW;pf.hc=kind?PF_LOW:0;
  pf.la=kind?0:32768;pf.ha=kind?32768:0;pf.ph=span-SL_RAMP-16;
  int32_t left[128],right[128];
  for(uint32_t block=0;block<3;block++){
   for(uint32_t i=0;i<128;i++){left[i]=12000;right[i]=-8000;}
   perf_begin(128);perf_filter_dry(left,right,128);
   if(!block)bad+=check("anti-click exit begins before the quantized stop",pf.end_split==16&&pf.end_mask==bit&&pf.act0==bit&&pf.act==bit);
  }
  int clean=1;for(uint32_t i=0;i<128;i++)if(left[i]!=12000||right[i]!=-8000)clean=0;
  bad+=check(kind?"HPF returns dry within the anti-click transition":"LPF returns dry within the anti-click transition",clean&&!pf.la&&!pf.ha);
 }
 if(!bad)puts("fx-release: all passed");return bad?1:0;
}
