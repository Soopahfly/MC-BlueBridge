import {readFileSync} from 'node:fs';
import vm from 'node:vm';
import assert from 'node:assert/strict';
const html=readFileSync(new URL('../tools/ds4-mouse-config.html',import.meta.url),'utf8');
const source=html.match(/<script type="module">([\s\S]*?)<\/script>/)[1];
class Element {value='';options=[];checked=false;disabled=false;textContent='';replaceChildren(...options){this.options=options;this.value=options[0]?.value??'';}}
const elements=Object.fromEntries(['connect','refresh','pointer-settings','status','touch-mouse','right-click','right-button','scroll','scroll-step','invert-scroll','save-pointer'].map(id=>[id,new Element()]));
let cfg={right_click:0,right_button:5,scroll:0,scroll_step:64,invert_scroll:0,enabled:false},active=JSON.stringify(cfg),supported=true,rejectSave=false;
let waiting,queue=[],stopped=false;const commands=[];
function reply(text){const event={value:new TextEncoder().encode(text),done:false};if(waiting){const resolve=waiting;waiting=null;resolve(event);}else queue.push(event);}
const reader={read:()=>queue.length?Promise.resolve(queue.shift()):stopped?Promise.resolve({done:true}):new Promise(resolve=>{waiting=resolve;}),releaseLock(){},async cancel(){stopped=true;if(waiting){waiting({done:true});waiting=null;}}};
const writer={releaseLock(){},async write(bytes){
  const [,id,cmd,...args]=new TextDecoder().decode(bytes).trim().split('|');commands.push([cmd,...args]);let response;
  if(cmd==='HELLO')response={capabilities:{ds4_mouse_gestures:supported}};
  else if(cmd==='DS4_MOUSE_GET')response=cfg;
  else if(cmd==='DS4_MOUSE_SET'){
    if(rejectSave){reply('BB1|'+id+'|ERR|Save rejected\n');return;}
    cfg={right_click:Number(args[0]),right_button:Number(args[1]),scroll:Number(args[2]),scroll_step:Number(args[3]),invert_scroll:Number(args[4]),enabled:args[5]==='1'};
    response={restart_required:JSON.stringify(cfg)!==active};
  }else throw new Error('Unexpected command '+cmd);
  const line='BB1|'+id+'|OK|'+JSON.stringify(response)+'\n';reply(line.slice(0,5));reply(line.slice(5));
}};
const port={readable:{getReader:()=>reader},writable:{getWriter:()=>writer},async open(){stopped=false;active=JSON.stringify(cfg);},async close(){}};
const context=vm.createContext({document:{getElementById:id=>elements[id]},navigator:{serial:{requestPort:async()=>port}},Option:class{constructor(label,value){this.label=label;this.value=value;}},TextEncoder,TextDecoder,setTimeout,clearTimeout,console});
vm.runInContext(source,context);await elements.connect.onclick();
assert.equal(elements['pointer-settings'].disabled,false);assert.equal(elements['right-click'].value,'0');assert.equal(elements['right-button'].disabled,true);
elements['right-click'].value='3';elements['right-click'].onchange();assert.equal(elements['right-button'].disabled,false);
elements['touch-mouse'].checked=true;elements['right-button'].value='5';elements.scroll.checked=true;elements['scroll-step'].value='32';elements['invert-scroll'].checked=true;
await elements['save-pointer'].onclick();assert.deepEqual(cfg,{right_click:3,right_button:5,scroll:1,scroll_step:32,invert_scroll:1,enabled:true});assert.equal(elements['pointer-settings'].disabled,true);assert.match(elements.status.textContent,/Reconnect/);
await elements.connect.onclick();assert.equal(elements['scroll-step'].value,'32');assert.equal(elements['invert-scroll'].checked,true);
await elements['save-pointer'].onclick();assert.match(elements.status.textContent,/No restart/);
for(const invalid of ['0','15','256','1.5','']){elements['scroll-step'].value=invalid;const before=commands.length;await elements['save-pointer'].onclick();assert.equal(commands.length,before);assert.match(elements.status.textContent,/16 to 255/);}
elements['scroll-step'].value='64';rejectSave=true;const before=JSON.stringify(cfg);await elements['save-pointer'].onclick();assert.equal(JSON.stringify(cfg),before);assert.match(elements.status.textContent,/Save rejected/);rejectSave=false;
elements['right-click'].value='1';elements['right-click'].onchange();assert.equal(elements['right-button'].disabled,true);await elements['save-pointer'].onclick();assert.equal(cfg.right_click,1);
supported=false;await elements.connect.onclick();assert.equal(elements['pointer-settings'].disabled,true);assert.match(elements.status.textContent,/gesture preview/);
await reader.cancel();console.log('Mouse GUI: settings/readback, conditional button selector, atomic save/restart, validation, failed save and capability fallback passed');
