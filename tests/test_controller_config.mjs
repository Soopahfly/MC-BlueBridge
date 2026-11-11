// Exercise the actual configuration page with a simulated Web Serial adapter.
import {readFileSync} from 'node:fs';
import vm from 'node:vm';
import assert from 'node:assert/strict';
const html=readFileSync(new URL('../tools/controller-config.html',import.meta.url),'utf8');
const source=html.match(/<script type="module">([\s\S]*?)<\/script>/)[1];
class Element {
  value=''; options=[]; checked=false; disabled=false; textContent='';
  replaceChildren(...options) { this.options=options; this.value=options[0]?.value ?? ''; }
}
const elements=Object.fromEntries(['connect','refresh','status','pointer-settings','pointer-status','touch-mouse','save-pointer','pointer-label','pointer-help','gyro-settings','gyro-enabled','gyro-sensitivity','gyro-activation','gyro-invert-x','gyro-invert-y','gyro-pause','gyro-fire'].map(id=>[id,new Element()]));
let steamSupport=true,gyroSupport=true;
let aim={enabled:0,sensitivity:100,activation:0,flags:0,pause_button:16};
let touchMouse=false;
let waiting, queue=[], stopped=false, saved, active=0, changedId=false;
const commands=[];
function reply(text) {
  const event={value:new TextEncoder().encode(text),done:false};
  if(waiting) { const resolve=waiting; waiting=null; resolve(event); } else queue.push(event);
}
const reader={
  read:()=>queue.length ? Promise.resolve(queue.shift()) : stopped ? Promise.resolve({done:true}) : new Promise(resolve=>{waiting=resolve;}),
  releaseLock:()=>{},
  cancel:async()=>{ stopped=true; if(waiting) { waiting({done:true}); waiting=null; } },
};
const writer={releaseLock:()=>{},write:async bytes=>{
  const [,id,cmd,...args]=new TextDecoder().decode(bytes).trim().split('|');
  commands.push([cmd,...args]);
  let response;
  if(cmd==='HELLO') response={capabilities:{touch_mouse:true,steam_ble:steamSupport,steam_gyro:gyroSupport}};
  else if(cmd==='STEAM_AIM_GET') response=aim;
  else if(cmd==='STEAM_AIM_SET'){[aim.enabled,aim.sensitivity,aim.activation,aim.flags,aim.pause_button]=args.map(Number);touchMouse=args[5]==='1';response={restart_required:aim.enabled===1||touchMouse};}
  else if(cmd==='TOUCH_MOUSE_GET') response={enabled:touchMouse,active:false,available:true};
  else if(cmd==='TOUCH_MOUSE_SET'){touchMouse=args[0]==='1';response={restart_required:touchMouse};}
  else throw new Error(`Unexpected command ${cmd}`);
  // Deliberately split a line across chunks to exercise stream reassembly.
  const line=`BB1|${id}|OK|${JSON.stringify(response)}\n`;
  reply(line.slice(0,5)); reply(line.slice(5));
}};
const port={readable:{getReader:()=>reader},writable:{getWriter:()=>writer},open:async()=>{stopped=false;},close:async()=>{}};
const context=vm.createContext({document:{getElementById:id=>elements[id]},navigator:{serial:{requestPort:async()=>port}},
  Option:class {constructor(label,value){this.label=label;this.value=value;}}, TextEncoder,TextDecoder,setTimeout,clearTimeout,console});
vm.runInContext(source,context);
await elements.connect.onclick();
await elements.connect.onclick();
assert.equal(elements['pointer-settings'].disabled,false);
elements['touch-mouse'].checked=true;await elements['save-pointer'].onclick();
assert.equal(touchMouse,true);assert.equal(elements['pointer-settings'].disabled,true);
assert.match(elements.status.textContent,/Reconnect/);

await elements.connect.onclick();assert.match(elements['pointer-label'].textContent,/Steam/);
assert.equal(elements['gyro-settings'].hidden,false);
assert.equal(elements['gyro-sensitivity'].value,'100');
elements['gyro-sensitivity'].value='0';
const before=commands.length;await elements['save-pointer'].onclick();
assert.equal(commands.length,before);assert.match(elements.status.textContent,/1 to 200/);
elements['gyro-sensitivity'].value='150';elements['gyro-enabled'].checked=true;
elements['gyro-activation'].value='2';elements['gyro-fire'].checked=true;elements['gyro-invert-y'].checked=true;elements['gyro-pause'].value='17';
await elements['save-pointer'].onclick();
assert.deepEqual(aim,{enabled:1,sensitivity:150,activation:2,flags:6,pause_button:17});
assert.equal(elements['pointer-settings'].disabled,true);
await elements.connect.onclick();assert.equal(elements['gyro-enabled'].checked,true);assert.equal(elements['gyro-sensitivity'].value,'150');
steamSupport=false;gyroSupport=false;await elements.connect.onclick();assert.match(elements['pointer-label'].textContent,/DualShock/);
assert.match(elements['pointer-help'].textContent,/does not include Steam/);
assert.equal(elements['gyro-settings'].hidden,true);
await reader.cancel();
console.log('config: mouse/gyro settings, validation, save/restart, reconnect and old-firmware fallback passed');
