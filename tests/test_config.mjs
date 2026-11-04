// Exercise the actual configuration page with a simulated Web Serial adapter.
import {readFileSync} from 'node:fs';
import vm from 'node:vm';
import assert from 'node:assert/strict';
const html=readFileSync(new URL('../tools/oled-config.html',import.meta.url),'utf8');
const source=html.match(/<script type="module">([\s\S]*?)<\/script>/)[1];
class Element {
  value=''; options=[]; checked=false; disabled=false; textContent='';
  replaceChildren(...options) { this.options=options; this.value=options[0]?.value ?? ''; }
}
const elements=Object.fromEntries(['connect','refresh','settings','controller','profile','enabled','fire','save','status'].map(id=>[id,new Element()]));
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
  if(cmd==='HELLO') response={capabilities:{profile_oled:true}};
  else if(cmd==='CONTROLLERS') response={selected_index:0,controllers:[{index:0,name:'DualSense'}]};
  else if(cmd==='CONTROLLER_SELECT') response='selected';
  else if(cmd==='PROFILES') response={active:0,labels:['South','East','West','North'],profiles:[{index:0,name:'Default'},{index:1,name:'Arcade'}]};
  else if(cmd==='PROFILE_GET') response={id:changedId?999:10+Number(args[0]),oled_pew_enabled:args[0]==='1',oled_fire_button:3};
  else if(cmd==='PROFILE_OLED') { saved=args; response='saved'; }
  else throw new Error(`Unexpected command ${cmd}`);
  // Deliberately split a line across chunks to exercise stream reassembly.
  const line=`BB1|${id}|OK|${JSON.stringify(response)}\n`;
  reply(line.slice(0,5)); reply(line.slice(5));
}};
const port={readable:{getReader:()=>reader},writable:{getWriter:()=>writer},open:async()=>{},close:async()=>{}};
const context=vm.createContext({document:{getElementById:id=>elements[id]},navigator:{serial:{requestPort:async()=>port}},
  Option:class {constructor(label,value){this.label=label;this.value=value;}}, TextEncoder,TextDecoder,setTimeout,clearTimeout,console});
vm.runInContext(source,context);
await elements.connect.onclick();
assert.equal(elements.settings.disabled,false);
assert.equal(elements.enabled.checked,false);
assert.equal(elements.fire.value,'3');
elements.profile.value='1'; await elements.profile.onchange();
assert.equal(elements.enabled.checked,true);
elements.enabled.checked=false; elements.fire.value='2';
await elements.save.onclick();
assert.deepEqual(saved,['1','0','2']);
assert.match(elements.status.textContent,/saved/);
assert.equal(commands.some(([cmd])=>cmd==='PROFILE_SELECT'),false);
saved=null; changedId=true;
await elements.save.onclick();
assert.equal(saved,null);
assert.match(elements.status.textContent,/Profile changed/);
await reader.cancel();
console.log('config: connection, split replies, profile selection, checkbox save and stale-profile protection passed');
