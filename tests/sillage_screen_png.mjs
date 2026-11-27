// Convert screenshots of the rendered firmware framebuffer, without dependencies.
import fs from 'node:fs';
import zlib from 'node:zlib';
const b=fs.readFileSync(process.argv[2]||'build/sound-design-screens.ppm');
const header=b.subarray(0,80).toString('ascii').match(/^P6\n(\d+) (\d+)\n255\n/);
if(!header)throw Error('Expected P6 screenshot');
const w=+header[1],h=+header[2],rgb=b.subarray(Buffer.byteLength(header[0]));
function crc(data){let c=0xffffffff;for(const x of data){c^=x;for(let k=0;k<8;k++)c=(c>>>1)^((c&1)?0xedb88320:0);}return(c^0xffffffff)>>>0;}
function chunk(type,data){const t=Buffer.from(type),n=Buffer.alloc(4),end=Buffer.alloc(4);n.writeUInt32BE(data.length);end.writeUInt32BE(crc(Buffer.concat([t,data])));return Buffer.concat([n,t,data,end]);}
const ih=Buffer.alloc(13);ih.writeUInt32BE(w);ih.writeUInt32BE(h,4);ih[8]=8;ih[9]=2;
const rows=Buffer.alloc((w*3+1)*h);for(let y=0;y<h;y++)rgb.copy(rows,y*(w*3+1)+1,y*w*3,(y+1)*w*3);
fs.writeFileSync(process.argv[3]||'build/sound-design-screens.png',Buffer.concat([Buffer.from([137,80,78,71,13,10,26,10]),chunk('IHDR',ih),chunk('IDAT',zlib.deflateSync(rows)),chunk('IEND',Buffer.alloc(0))]));
