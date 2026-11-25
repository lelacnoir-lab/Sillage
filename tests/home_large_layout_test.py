#!/usr/bin/env python3
"""Exercise the actual graph_names C function with traced display calls.

Checks every selected track, both footer heights, mute state, and the four
rendering branches. Detects the old off-screen T7/T8 layout. No audio stubs.
"""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

def test_source(path):
    source = Path(path).read_text(encoding="utf-8")
    begin = source.index("static void graph_names(void)")
    end = source.index("\n/*", begin)
    function = source[begin:end]
    return r'''
typedef unsigned int uint32_t;
typedef int int32_t;
typedef unsigned short uint16_t;
#ifndef NTRK
#define NTRK 8u
#endif
enum { P_MUTE, T_BG=100, T_SURF, T_INK, T_TEXT, T_DIM };
#define T_TRK(k) (200+(k))
typedef struct { int p[1]; } track_t;
static track_t trk[NTRK];
static struct { uint32_t sel; } song;
static struct { int quark, crypt, neon; } ux;
static int height, errors, seen[NTRK], names[NTRK], selected;
static const int AF_M=0;
#define CAP_IN(F,h) (((h)-12+1)/2-4)
static int graph_h(void) { return height; }
static uint16_t ux_mix(int a,int b,int c) { (void)b;(void)c;return a; }
static void cv_rect(int x,int y,int w,int h,int col) {
    (void)col;
    if(x<0||y<0||w<0||h<12||x+w>240||y+h>height) {
        /* One-pixel QUARK rails are valid too. */
        if(x<0||y<0||w<0||h<1||x+w>240||y+h>height) errors++;
    }
}
static void cr_stone(int x,int y,int w,int h,int c) {cv_rect(x,y,w,h,c);}
static void ne_hgrad(int x,int y,int w,int h,const uint16_t*s,int n) {(void)n;cv_rect(x,y,w,h,s[0]);}
#define NE_VIOLET 300
static void cv_text_on(int x,int y,const int*f,const char*d,int ink,int bg) {
    int k=d[0]-'1';(void)f;(void)ink;
    if(k<0||k>=NTRK) {errors++;return;}
    seen[k]++;
    if(x<0||x+8>240||y+4<0||y+16>height) errors++;
    if(k==song.sel&&bg==T_TRK(k))selected++;
}
static void sound_name(const track_t*t,char*b) {b[0]='A'+(int)(t-trk);b[1]=0;}
static void cv_free_text(int x,int y,const int*f,const char*b,int ink,int bg,int width) {
    int k=b[0]-'A';(void)f;(void)ink;(void)bg;
    if(k<0||k>=NTRK) {errors++;return;}names[k]++;
    if(x<0||x+width>240||width<80||y+4<0||y+16>height) errors++;
}
''' + function + r'''
int main(void) {
    for(int hi=0;hi<2;hi++)for(int theme=0;theme<4;theme++)for(int mute=0;mute<2;mute++)for(int sel=0;sel<NTRK;sel++) {
        height=hi?78:62;song.sel=sel;selected=0;
        ux.quark=theme==1;ux.crypt=theme==2;ux.neon=theme==3;
        for(int k=0;k<NTRK;k++){seen[k]=names[k]=0;trk[k].p[P_MUTE]=mute&&k!=sel;}
        graph_names();
        for(int k=0;k<NTRK;k++)if(seen[k]!=1||names[k]!=1)errors++;
        if(selected!=1)errors++;
    }
    return errors?1:0;
}
'''

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", default=str(ROOT/"firmware/src/ui_graph.c"))
    ap.add_argument("--emit")
    args = ap.parse_args()
    code = test_source(args.source)
    if args.emit:
        Path(args.emit).write_text(code, encoding="utf-8")
        return
    with tempfile.TemporaryDirectory() as tmp:
        src=Path(tmp)/"home.c"
        src.write_text(code, encoding="utf-8")
        for tracks in (4,6,8):
            exe=Path(tmp)/f"home-{tracks}"
            subprocess.run([*shlex.split(os.environ.get("CC","cc")),"-std=c99","-O1",f"-DNTRK={tracks}",str(src),"-o",str(exe)],check=True)
            subprocess.run([str(exe)],check=True)
    print("HOME LARGE: all track labels, names and selections stay on-screen (4/6/8 tracks; two heights; four render branches; muted/unmuted).")

if __name__=="__main__":
    main()
