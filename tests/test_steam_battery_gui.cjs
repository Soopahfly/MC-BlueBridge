const fs=require('fs'),vm=require('vm'),assert=require('assert'),path=require('path');
const elements={};const $=id=>elements[id]??={checked:false,disabled:false,value:'',textContent:'',
  classList:{toggle(k,v){this[k]=v}},addEventListener(){},replaceChildren(){}};
let controller={index:0,id:1,kind:18,connected:true};const calls=[],messages=[];
const initial=()=>({detected:true,capabilities:{steam_features:true,steam_battery:true},
  steam_battery:{led:false,haptics:false,strength:35,threshold_mv:2200},
  steam_battery_status:{valid:true,millivolts:2351,low:false,controller_index:0,test_available:true}});
const c={$ ,selectedAdapterId:'first',selectedController:()=>controller,offlineManagement:false,
  controllerByIndex:()=>controller,controllersData:{connected_index:0},statusData:initial(),
  Option:function(name,value){this.value=value},setDS4TabVisible(){},guardAdapterAction:f=>f,
  toast:(...args)=>messages.push(args),setTimeout(){},refresh(){},
  post:async(url,payload)=>{calls.push({url,payload});return {settings:payload}}};
vm.createContext(c);vm.runInContext(fs.readFileSync(path.join(__dirname,'../build-host/steam-battery-ui.js'),'utf8'),c);
const run=s=>vm.runInContext(s,c);
(async()=>{
  run('renderSteamBattery()');assert(!$('steamBatterySection').classList.hidden&&!$('steamBatteryTest').disabled);
  assert(!$('steamBatteryLED').checked&&!$('steamBatteryHaptics').checked&&$('steamBatteryReading').textContent.includes('2.35 V'));
  $('steamBatteryLED').checked=true;$('steamBatteryHaptics').checked=true;$('steamBatteryVoltage').value='2.15';
  run('steamBatteryDirty=true;renderSteamBattery()');assert($('steamBatteryLED').checked);
  await run('saveSteamBattery()');assert.deepEqual(calls[0],{url:'/api/bluebridge/steam-battery',payload:{led:true,haptics:true,strength:35,threshold_mv:2150}});
  const saved=JSON.stringify(c.statusData.steam_battery);await run('testSteamBattery()');
  assert.deepEqual(calls[1],{url:'/api/bluebridge/steam-battery/test',payload:{}});
  assert.equal(JSON.stringify(c.statusData.steam_battery),saved);
  assert(c.statusData.steam_battery_status.millivolts===2351&&!c.statusData.steam_battery_status.low);
  $('steamBatteryVoltage').value='1.7';const before=calls.length;await run('saveSteamBattery()');
  assert(calls.length===before&&messages.at(-1)[1]);
  $('steamBatteryVoltage').value='2.1';$('steamBatteryLED').checked=false;run('steamBatteryDirty=true');
  c.post=async()=>{throw Error('offline')};await run('saveSteamBattery()');assert(!$('steamBatteryLED').checked&&run('steamBatteryDirty'));
  c.statusData.steam_battery_status.valid=false;run('renderSteamBattery()');assert($('steamBatteryReading').textContent.includes('suppressed'));
  c.statusData.steam_battery_status.controller_index=1;run('renderSteamBattery()');assert($('steamBatteryTest').disabled);
  c.selectedAdapterId='second';c.statusData=initial();run('renderSteamBattery()');assert(!run('steamBatteryDirty'));
  c.statusData.steam_battery=undefined;run('renderSteamBattery()');assert($('steamBatterySave').disabled);
  c.statusData.capabilities={};run('renderSteamBattery()');assert($('steamBatterySection').classList.hidden);
  c.statusData=initial();run('renderSteamBattery()');let finish;c.post=()=>new Promise(resolve=>finish=resolve);
  const pending=run('saveSteamBattery()');c.selectedAdapterId='third';c.statusData=initial();
  finish({settings:{led:true,haptics:true,strength:40,threshold_mv:2100}});await pending;
  assert(!c.statusData.steam_battery.led&&c.statusData.steam_battery.threshold_mv===2200);
  controller={kind:4,connected:true};run('renderSteamBattery()');assert($('steamBatterySection').classList.hidden);
  console.log('Steam battery UI: defaults, voltage display, save/test, independent settings, stale data, validation, failures and adapter/controller isolation passed');
})().catch(e=>{console.error(e);process.exitCode=1});
