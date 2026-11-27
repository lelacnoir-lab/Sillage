/* Record physical LCD transfers, including the intermediate blank frame. */
#define UI_TEST_NO_MAIN 1
static unsigned int audio_peak_us;
#include "ui_test.c"
int main(void)
{
 int bad=0;ui_power_on();frame();
 for(uint32_t i=0;i<NPAGES;i++) if(PAGES[i].graph==GR_EVOLVE){ui.home=0;ui.page=(uint8_t)i;break;}
 set_engine_of(TSEL,0);page_entered();
 host_fullscreen_fills=0;frame();
 bad+=check("enter EVOLVE without blanking the physical screen",host_fullscreen_fills==0);
 for(uint32_t k=0;k<3;k++) {
  host_fullscreen_fills=0;host_blit_rows=0;
  turn(k==0?EN_K1:k==1?EN_K3:EN_K4,1);
  bad+=check("knob refresh blits completed strips without a blank fill",host_fullscreen_fills==0 && host_blit_rows==240);
 }
 /* Full coverage must also remove pixels left by another page/overlay. */
 uint16_t expected[240*240];memcpy(expected,host_screen,sizeof expected);
 for(uint32_t i=0;i<240*240;i++)host_screen[i]=0xffff;
 ui.force=1;host_fullscreen_fills=0;frame();
 bad+=check("completed strips cover all pixels; no stale old-page content",!host_fullscreen_fills&&!memcmp(expected,host_screen,sizeof expected));
 host_blit_rows=0;host_fullscreen_fills=0;frame();
 bad+=check("unchanged EVOLVE frame sends no LCD refresh",!host_blit_rows&&!host_fullscreen_fills);
 if(!bad)puts("EVOLVE no-flash: all passed");return bad?1:0;
}
