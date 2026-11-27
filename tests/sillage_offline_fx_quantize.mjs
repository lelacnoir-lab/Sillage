import fs from 'node:fs';
import assert from 'node:assert/strict';
const html=fs.readFileSync('../../outputs/Sillage V3 test local/Sillage V3 local.html','utf8');
const payload=html.match(/Promise.resolve\(Uint8Array.from\(atob\("([^"]+)"/)[1];
const {instance}=await WebAssembly.instantiate(Buffer.from(payload,'base64'),{});
const e=instance.exports;e._initialize();e.web_nor_erase();e.web_boot();
function render(ms){for(let n=Math.round(ms*44.1);n>0;n-=128)e.web_render(Math.min(128,n));}
function tap(b){e.web_buttons(1<<b);render(60);e.web_buttons(0);render(60);}
function key(k){e.web_keys(1<<k);render(60);e.web_keys(0);render(60);}
render(3400);e.web_test_heavy();
e.web_buttons(1);render(700);e.web_buttons(0);render(60); // actual FX hold locks the panel
e.web_enc(4,3);render(100); // K2: default 1/4 -> 2 BAR
tap(10);render(180);assert(e.web_playing());
key(0);const a=e.web_lit_keys()&1;render(250);const b=e.web_lit_keys()&1;
assert.notEqual(a,b,'FX start must flash while waiting on 2 BAR');
render(4100);assert(e.web_lit_keys()&1,'FX key is steady after quantized start');
render(250);assert(e.web_lit_keys()&1,'FX key remains steady');
key(0);const c=e.web_lit_keys()&1;render(250);const d=e.web_lit_keys()&1;
assert.notEqual(c,d,'FX stop must flash while waiting on 2 BAR');
render(4100);assert.equal(e.web_lit_keys()&1,0,'FX key is off after quantized stop');
e.web_keys(1<<23);render(60);
assert.equal(e.web_test_fill_active(),1,'E5 activates Fill immediately on FX HOLD');
assert(e.web_lit_keys()&(1<<23),'Fill key lights while held');
const fb=new Uint16Array(e.memory.buffer,e.web_screen(),240*240),rgb=Buffer.alloc(240*240*3);
for(let i=0;i<fb.length;i++){const p=(fb[i]>>8)|((fb[i]&255)<<8);rgb[3*i]=(p>>11)*255/31;rgb[3*i+1]=((p>>5)&63)*255/63;rgb[3*i+2]=(p&31)*255/31;}
fs.writeFileSync('build/fx-quantize-real.ppm',Buffer.concat([Buffer.from('P6\n240 240\n255\n'),rgb]));
e.web_keys(0);render(60);assert.equal(e.web_test_fill_active(),0,'Fill is momentary even with latched effects');
console.log('PASS: packaged V3, actual held FX panel, K2=2 BAR, latched key start and stop both queued/blinking then applied.');
