import fs from 'node:fs';
const html=fs.readFileSync('../../outputs/Sillage V3 test local/Sillage - SOUND DESIGN local.html','utf8');
const payload=html.match(/Promise.resolve\(Uint8Array.from\(atob\("([^"]+)"/)[1];
const {instance}=await WebAssembly.instantiate(Buffer.from(payload,'base64'),{});
const e=instance.exports;e._initialize();e.web_nor_erase();e.web_boot();
function render(ms){for(let n=Math.round(ms*44.1);n>0;n-=128)e.web_render(Math.min(128,n));}
function tap(b){e.web_buttons(1<<b);render(60);e.web_buttons(0);render(60);}
function turn(role,n){e.web_enc(role,n);render(80);}
function title(){const u=new Uint8Array(e.memory.buffer),p=e.web_test_page_title();let n=p;while(u[n])n++;return new TextDecoder().decode(u.subarray(p,n));}
function check(ok,msg){if(!ok)throw Error(msg+' (page '+title()+')');console.log('offline: '+msg+' OK');}
const shots=[];
function shot(){shots.push(new Uint16Array(e.memory.buffer,e.web_screen(),240*240).slice());}
function petrol(){return new Uint16Array(e.memory.buffer,e.web_screen(),240*240).includes(0xEF11);}
render(3400);tap(4);
for(let i=0;i<80&&title()!=='PRESET INIT';i++)tap(4);
check(title()==='PRESET INIT','physical EDIT reaches PRESET INIT');
shot();turn(3,1);tap(4);turn(3,1);shot();
check(petrol(),'engine confirmation enters saturated-blue SOUND DESIGN');
for(let i=0;i<80&&title()!=='SAVE SOUND';i++)turn(0,1);
check(title()==='SAVE SOUND','bare engine edit -> SELECT reaches SAVE SOUND');
shot();tap(13);check(title()==='USER','OCT+ opens USER destination page');shot();
check(!petrol(),'USER destination page restores the normal colour');
turn(3,1);tap(13);check(e.web_test_name_open()===1,'OCT+ opens naming after choosing a destination');
tap(13);check(e.web_test_user_preset(1)===1&&e.web_test_name_open()===0,'final OCT+ stores the chosen user preset');
check(!petrol(),'saving leaves the creation colour');
const rgb=Buffer.alloc(240*shots.length*240*3);
for(let n=0;n<shots.length;n++)for(let y=0;y<240;y++)for(let x=0;x<240;x++){
 const raw=shots[n][y*240+x],p=(raw>>8)|((raw&255)<<8),i=(y*240*shots.length+n*240+x)*3;
 rgb[i]=Math.round((p>>11)*255/31);rgb[i+1]=Math.round(((p>>5)&63)*255/63);rgb[i+2]=Math.round((p&31)*255/31);
}
fs.writeFileSync('build/sound-design-screens.ppm',Buffer.concat([Buffer.from(`P6\n${240*shots.length} 240\n255\n`),rgb]));
