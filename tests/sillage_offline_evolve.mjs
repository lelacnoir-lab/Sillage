import fs from 'node:fs';
import assert from 'node:assert/strict';
const html=fs.readFileSync('../../outputs/Sillage V3 test local/Sillage V3 local.html','utf8');
const payload=html.match(/Promise.resolve\(Uint8Array.from\(atob\("([^"]+)"/)[1];
const {instance}=await WebAssembly.instantiate(Buffer.from(payload,'base64'),{});
const e=instance.exports;e._initialize();e.web_nor_erase();e.web_boot();
function render(ms){for(let n=Math.round(ms*44.1);n>0;n-=128)e.web_render(Math.min(128,n));}
function tap(b){e.web_buttons(1<<b);render(60);e.web_buttons(0);render(60);}
function turn(k,n){e.web_enc(k,n);render(60);}
function title(){const u=new Uint8Array(e.memory.buffer),p=e.web_test_page_title();let n=p;while(u[n])n++;return new TextDecoder().decode(u.subarray(p,n));}
function screen(){const fb=new Uint16Array(e.memory.buffer,e.web_screen(),240*240),rgb=Buffer.alloc(240*240*3);for(let i=0;i<fb.length;i++){const p=(fb[i]>>8)|((fb[i]&255)<<8);rgb[3*i]=(p>>11)*255/31;rgb[3*i+1]=((p>>5)&63)*255/63;rgb[3*i+2]=(p&31)*255/31;}return rgb;}
render(3400);e.web_test_evolve_phrase();tap(9);
for(let i=0;i<12 && title()!=='EVOLVE';i++)turn(0,1);
assert.equal(title(),'EVOLVE');
turn(3,45);assert.equal(e.web_test_evolve_want(0),45);assert.equal(e.web_test_evolve_live(0),45);
turn(5,1); // K3: octave +1 for subsequent additions
assert.equal(e.web_test_evolve_options(0)&3,1);
for(const next of [2,0]) { turn(5,1); assert.equal(e.web_test_evolve_options(0)&3,next); }
turn(5,-1);assert.equal(e.web_test_evolve_options(0)&3,2); // left of 0 reaches -1
turn(5,2);assert.equal(e.web_test_evolve_options(0)&3,1);
turn(3,10);
turn(6,-25); // K4: 25% of a step
assert.equal((e.web_test_evolve_options(0)>>8)&255,25);
const screenshot=screen();fs.writeFileSync('build/evolve-real.ppm',Buffer.concat([Buffer.from('P6\n240 240\n255\n'),screenshot]));
// ALGORITHM selects every track, retaining independent settings including T8.
for(let i=0;i<7;i++)turn(1,1);
turn(3,60);assert.equal(e.web_test_evolve_want(7),60);assert.equal(e.web_test_evolve_want(0),55);
assert.equal(e.web_test_evolve_options(7),0);
tap(12);assert.equal(e.web_test_evolve_want(7),0);assert.equal(e.web_test_evolve_want(0),55);
tap(13);for(let i=0;i<8;i++)assert.equal(e.web_test_evolve_want(i),0);
for(let i=0;i<7;i++)turn(1,-1);
tap(10);render(200);turn(3,80);
assert.equal(e.web_test_evolve_want(0),80);assert.equal(e.web_test_evolve_live(0),0);
render(2300);assert.equal(e.web_test_evolve_live(0),80);
turn(6,50);assert.equal((e.web_test_evolve_options(0)>>8)&255,75);
assert.equal((e.web_test_evolve_options(0)>>16)&255,25);
render(2300);assert.equal((e.web_test_evolve_options(0)>>16)&255,75);
tap(12);assert.equal(e.web_test_evolve_want(0),0);assert.equal(e.web_test_evolve_live(0),80);
render(2300);assert.equal(e.web_test_evolve_live(0),0);
tap(10);tap(0);assert.notEqual(title(),'EVOLVE');tap(6);assert.equal(title(),'HOME');
console.log('PASS exact V3 HTML: K1/K3/K4, independent tracks, reset, next-loop density/length, original pages, real screen.');
