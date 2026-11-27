/* Real local V3 code: input, transport grid, LEDs and preserved patch data. */
#define UI_TEST_NO_MAIN 1
static unsigned int audio_peak_us;
#include "ui_test.c"
int main(void)
{
 int bad=0;
 ui_power_on();frame();
 press(B_ENV);
 bad+=check("ENV tap retains envelopes", !live_transpose.open && cur_fam()==FAM_ENV);
 hold(B_ENV);
 bad+=check("ENV hold opens real transpose and latches after release",live_transpose.open);
 int16_t params[P_COUNT];memcpy(params,TSEL->p,sizeof params);
 step_t steps[NSTEP];memcpy(steps,TSEL->step,sizeof steps);
 host_notes=1u<<16;frame();
 bad+=check("physical key applies +4 while stopped",live_transpose.active==4&&!live_transpose.pending);
 bad+=check("transposition leaves patch and notes intact",!memcmp(params,TSEL->p,sizeof params)&&!memcmp(steps,TSEL->step,sizeof steps));
 turn(EN_K3,-4);bad+=check("knob 3 selects immediate",live_transpose.grid==0);
 song.playing=1;transpose_request(-2);
 bad+=check("NOW applies without waiting",live_transpose.active==-2&&!live_transpose.pending);
 live_transpose.grid=4;launch_steps=5;transpose_request(7);
 bad+=check("BAR queues for next measure",live_transpose.active==-2&&live_transpose.pending&&live_transpose.at==16);
 uint32_t breath;fm1_ms=0;uint32_t off=key_leds(&breath);fm1_ms=250;uint32_t on=key_leds(&breath);
 bad+=check("queued key blinks; current key stays lit",!(off&(1u<<19))&&(on&(1u<<19))&&(on&(1u<<10)));
 for(uint32_t q=1;q<=4;q++){live_transpose.grid=(uint8_t)q;launch_steps=5;transpose_request(3);bad+=check("request aligns with selected transport grid",live_transpose.at>5&&live_transpose.at%TP_GRID[q]==0);}
 seq_stop();bad+=check("STOP cancels pending request",!live_transpose.pending);
 press(B_ENV);bad+=check("ENV tap closes transpose",!live_transpose.open);
 hold(B_ENV);press(B_HOME);bad+=check("HOME closes transpose and returns home",!live_transpose.open&&ui.home);
 if(!bad)puts("transpose: all passed");return bad?1:0;
}
