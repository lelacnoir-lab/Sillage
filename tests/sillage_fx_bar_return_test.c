#define UI_TEST_NO_MAIN 1
static unsigned int audio_peak_us;
#include "ui_test.c"

static void reset_return(void)
{
 memset(&pf,0,sizeof pf);pf.src=pf.next=PF_N;pf.lc=PF_TOP;pf.gg=32768;
 for(uint32_t k=0;k<NTRK;k++)pf.mg[k]=32768;
 memset((void*)perf_k,0,sizeof perf_k);perf_held=perf_act=perf_latched=0;
 perf_kill=perf_cut=perf_solo=perf_latch_on=0;sl_lent=0;perf_quant=0;
 song.playing=1;song.g[G_BPM]=120;
}
int main(void)
{
 int bad=0;ui_power_on();uint32_t missed=0;
 for(uint32_t grid=1;grid<6;grid++) for(uint32_t effect=0;effect<PF_M1;effect++) {
  reset_return();perf_press(effect,1);
  int32_t l[32],r[32],send[32];
  for(uint32_t at=0;at<44100;at+=32) {
   for(uint32_t i=0;i<32;i++){l[i]=12000;r[i]=-9000;send[i]=0;}
   perf_begin(32);perf_dub(l,r,send,32);perf_filter_dry(l,r,32);perf_block(l,r,32);
  }
  if(effect==PF_LPF){pf.la=32768;pf.lc=PF_LOW;}
  if(effect==PF_HPF){pf.ha=32768;pf.hc=PF_LOW;}
  if(effect==PF_GATE)pf.gg=0;
  if(effect==PF_DUB)pf.dd=32768;
  perf_quant=(uint8_t)grid;pf.ph=(beat_samples()>>2)*FXQ_STEPS[grid]-16u;perf_press(effect,0);
  for(uint32_t i=0;i<32;i++){l[i]=i==16?60000:12000;r[i]=-l[i];send[i]=0;}
  perf_begin(32);perf_dub(l,r,send,32);perf_filter_dry(l,r,32);perf_block(l,r,32);
  if((perf_act&PF_BIT(effect))||l[16]!=60000||r[16]!=-60000||(effect==PF_DUB&&send[16])) {
   missed++;printf("late bar release misses first kick: FX %u, out %d, active %u\n",effect,l[16],perf_act);
  }
 }
 bad+=check("70 FX/grid pairs: late release leaves first kick fully dry",!missed);
 /* Real transport, including a tempo change: a kick at track step 1, not a
  * hand-set FX clock. Request release just before the bar, after tempo change. */
 reset_return();song.g[G_BPM]=117;trk[0].p[P_SLEN]=16;trk[0].p[P_SDIV]=2;
 seq_start();perf_quant=4;perf_press(PF_LPF,1);
 uint32_t requested=0, checked=0, previous=15;
 for(uint32_t at=0;at<450000;at+=32) {
  if(at==64000)song.g[G_BPM]=133;
  events_block(32);
  uint32_t phase=(launch_steps%16u)*(beat_samples()>>2)+launch_pos;
  uint32_t remain=(beat_samples()>>2)*16u-phase;
  if(at>100000 && !requested && remain>0 && remain<64) {perf_press(PF_LPF,0);requested=1;}
  int kick=requested && trk[0].seq_idx==0 && previous!=0;
  int32_t l[32],r[32];for(uint32_t i=0;i<32;i++){l[i]=kick&&i==0?60000:12000;r[i]=-l[i];}
  perf_begin(32);perf_filter_dry(l,r,32);perf_block(l,r,32);
  if(kick){checked=1;bad+=check("real sequencer first beat remains dry after a tempo change",l[0]==60000&&r[0]==-60000&&!(perf_act&PF_BIT(PF_LPF)));break;}
  previous=trk[0].seq_idx;
 }
 bad+=check("real transport bar test reached its target",requested&&checked);
 if(!bad)puts("bar-return: all passed");return bad?1:0;
}
