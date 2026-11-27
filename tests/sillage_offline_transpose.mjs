import fs from 'node:fs';
import assert from 'node:assert/strict';
const html=fs.readFileSync('../../outputs/Sillage V3 test local/Sillage V3 local.html','utf8');
const payload=html.match(/Promise.resolve\(Uint8Array.from\(atob\("([^"]+)"/)[1];
const {instance}=await WebAssembly.instantiate(Buffer.from(payload,'base64'),{});
const e=instance.exports;e._initialize();e.web_nor_erase();e.web_boot();
function render(ms){for(let n=Math.round(ms*44.1);n>0;n-=128)e.web_render(Math.min(128,n));}
function tap(b){e.web_buttons(1<<b);render(60);e.web_buttons(0);render(60);}
function title(){const u=new Uint8Array(e.memory.buffer),p=e.web_test_page_title();let n=p;while(u[n])n++;return new TextDecoder().decode(u.subarray(p,n));}
render(3400);tap(2);assert.notEqual(title(),'TRANSPOSE');
e.web_buttons(1<<2);render(650);e.web_buttons(0);render(60);assert.equal(title(),'TRANSPOSE');
e.web_keys(1<<16);render(60);e.web_keys(0);render(60);assert.equal(e.web_test_transpose_active(),4);
e.web_test_heavy();tap(10);render(180);
e.web_keys(1<<19);render(60);e.web_keys(0);render(60);assert.equal(e.web_test_transpose_pending(),1);assert.equal(e.web_test_transpose_active(),4);
render(2200);assert.equal(e.web_test_transpose_active(),7);assert.equal(e.web_test_transpose_pending(),0);
assert(e.web_lit_keys()&(1<<19));
const fb=new Uint16Array(e.memory.buffer,e.web_screen(),240*240),rgb=Buffer.alloc(240*240*3);
for(let i=0;i<fb.length;i++){const p=(fb[i]>>8)|((fb[i]&255)<<8);rgb[3*i]=(p>>11)*255/31;rgb[3*i+1]=((p>>5)&63)*255/63;rgb[3*i+2]=(p&31)*255/31;}
fs.writeFileSync('build/transpose-real.ppm',Buffer.concat([Buffer.from('P6\n240 240\n255\n'),rgb]));
tap(2);assert.notEqual(title(),'TRANSPOSE');
console.log('PASS: exact V3 HTML contains working real firmware: ENV hold/tap, actual keyboard transpose, next-bar application, steady key LED, original pages restored.');
