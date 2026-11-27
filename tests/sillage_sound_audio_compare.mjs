import fs from 'node:fs';
const paths=['../sillage-v3-sound-baseline/build/baseline.wasm','build/sillage-sound-local.wasm'];
async function device(path){const {instance}=await WebAssembly.instantiate(fs.readFileSync(path),{});const e=instance.exports;e._initialize();e.web_nor_erase();e.web_boot();return e;}
const [a,b]=await Promise.all(paths.map(device));
let frames=0;
function render(ms,compare=true){for(let n=Math.round(ms*44.1);n>0;n-=128){const m=Math.min(128,n);a.web_render(m);b.web_render(m);if(compare){for(const name of ['web_out_l','web_out_r']){const x=new Float32Array(a.memory.buffer,a[name](),m),y=new Float32Array(b.memory.buffer,b[name](),m);for(let i=0;i<m;i++)if(x[i]!==y[i])throw Error(`Audio changed at frame ${frames+i}, ${name}: ${x[i]} / ${y[i]}`);}frames+=m;}}}
render(3400);
for(const e of [a,b])e.web_test_heavy();
for(const e of [a,b])e.web_buttons(1<<10);render(60);
for(const e of [a,b])e.web_buttons(0);render(3000);
for(const e of [a,b])e.web_buttons(1<<10);render(60);
for(const e of [a,b])e.web_buttons(0);render(1500);
for(const e of [a,b])e.web_midi(0x90,60,100);render(500);
for(const e of [a,b])e.web_midi(0x80,60,0);render(1500);
console.log(`Audio unchanged: ${frames} stereo frames identical to the source baseline (boot, heavy sequence, stop/tails, MIDI note).`);
