const fs=require('fs'),vm=require('vm'),assert=require('assert'),path=require('path');
const elements={};const $=id=>elements[id]??={checked:false,disabled:false,value:'',textContent:'',
  classList:{toggle(k,v){this[k]=v}},addEventListener(){},replaceChildren(){}};
let controller={index:0,id:1,kind:18,connected:true};const calls=[],messages=[];
const initial=()=>({detected:true,output_mode:1,multi_controller:false,capabilities:{steam_features:true},
  touch_mouse:{enabled:false,active:false,available:true},steam_aim:{enabled:0,sensitivity:100,activation:0,flags:0,pause_button:16},
  steam_features:{right_click:false,haptics:false,haptic_gain:50,led_brightness:100}});
const c={$ ,selectedAdapterId:'first',selectedController:()=>controller,offlineManagement:false,
  controllerByIndex:()=>controller,controllersData:{connected_index:0},statusData:initial(),
  Option:function(name,value){this.value=value},setDS4TabVisible:(n,v)=>$(n+'Tab').classList.toggle('hidden',!v),
  guardAdapterAction:f=>f,toast:(...args)=>messages.push(args),setTimeout:()=>{},refresh:()=>{},
  post:async(url,payload)=>{calls.push({url,payload});return {settings:payload,mouse:{restart_required:true}}}};
vm.createContext(c);vm.runInContext(fs.readFileSync(path.join(__dirname,'../build-host/steam-ui.js'),'utf8'),c);
const run=s=>vm.runInContext(s,c);
(async()=>{
  run('renderSteam()');assert(!$('steamTab').classList.hidden&&!$('steamRightClick').checked&&!$('steamHaptics').checked);
  assert.equal($('steamGain').value,'50');assert.equal($('steamLED').value,'100');
  $('steamRightClick').checked=true;$('steamLED').value='30';run('steamFeedbackDirty=true;renderSteam()');
  assert($('steamRightClick').checked);await run('saveSteamFeedback()');
  assert.deepEqual(calls[0],{url:'/api/bluebridge/steam-features',payload:{right_click:true,haptics:false,haptic_gain:50,led_brightness:30}});
  $('steamMouse').checked=true;$('steamGyro').checked=true;$('steamSensitivity').value='125';
  $('steamActivation').value='1';$('steamPause').value='16';$('steamInvertX').checked=true;
  run('steamPointerDirty=true');await run('saveSteamPointer()');
  assert.deepEqual(calls[1],{url:'/api/bluebridge/steam-pointer',payload:{enabled:true,steam_aim:{enabled:true,sensitivity:125,activation:1,flags:1,pause_button:16}}});
  assert(c.statusData.steam_aim.restart_required);
  const before=calls.length;$('steamSensitivity').value='201';await run('saveSteamPointer()');
  assert.equal(calls.length,before);assert(messages.at(-1)[1]);
  c.post=async()=>{throw Error('offline')};$('steamRightClick').checked=false;run('steamFeedbackDirty=true');
  await run('saveSteamFeedback()');assert(!$('steamRightClick').checked&&run('steamFeedbackDirty'));
  c.selectedAdapterId='second';c.statusData=initial();run('renderSteam()');assert(!run('steamFeedbackDirty'));
  c.statusData.steam_features=undefined;run('renderSteam()');assert($('steamFeedbackSave').disabled&&!$('steamPointerSave').disabled);
  c.statusData.capabilities={};run('renderSteam()');assert($('steamTab').classList.hidden);
  c.statusData=initial();run('renderSteam()');let finish;c.post=()=>new Promise(resolve=>finish=resolve);
  $('steamRightClick').checked=true;const pending=run('saveSteamFeedback()');
  c.selectedAdapterId='third';c.statusData=initial();finish({settings:{right_click:true}});await pending;
  assert(!c.statusData.steam_features.right_click);
  controller={kind:4,connected:true};run('renderSteam()');assert($('steamTab').classList.hidden);
  console.log('Steam UI: defaults, controller gating, dirty refresh, both saves, validation/failure, missing reads and adapter-switch race passed');
})().catch(e=>{console.error(e);process.exitCode=1});
