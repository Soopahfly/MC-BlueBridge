const fs=require('fs'),vm=require('vm'),assert=require('assert'),path=require('path');
const elements={};const $=id=>elements[id]??={checked:false,disabled:false,textContent:'',classList:{toggle(k,v){this[k]=v}}};
let controller={index:0,id:1};const calls=[];
const c={$ ,selectedAdapterId:'first',selectedDS4:()=>controller,
 statusData:{capabilities:{ds4_right_click:true},ds4_mouse:{right_click:false}},
 guardAdapterAction:f=>f,toast:()=>{},post:async(url,payload)=>{calls.push({url,payload});return {settings:payload}}};
vm.createContext(c);vm.runInContext(fs.readFileSync(path.join(__dirname,'../build-host/right-click-ui.js'),'utf8'),c);
const run=s=>vm.runInContext(s,c);
(async()=>{
 run('renderDS4RightClick()');assert(!$('ds4RightClickEnabled').checked&&!$('ds4RightClickSave').disabled);
 $('ds4RightClickEnabled').checked=true;run('ds4RightClickDirty=true;renderDS4RightClick()');
 assert($('ds4RightClickEnabled').checked);await run('saveDS4RightClick()');
 assert.deepEqual(calls[0],{url:'/api/bluebridge/ds4-mouse',payload:{right_click:true}});
 assert(c.statusData.ds4_mouse.right_click);
 c.post=async()=>{throw Error('offline')};$('ds4RightClickEnabled').checked=false;run('ds4RightClickDirty=true');
 await run('saveDS4RightClick()');assert(!$('ds4RightClickEnabled').checked&&run('ds4RightClickDirty'));
 c.selectedAdapterId='second';c.statusData.ds4_mouse={right_click:true};run('renderDS4RightClick()');
 assert($('ds4RightClickEnabled').checked&&!run('ds4RightClickDirty'));
 c.statusData.ds4_mouse=undefined;run('renderDS4RightClick()');assert($('ds4RightClickSave').disabled);
 c.statusData.capabilities={};run('renderDS4RightClick()');assert($('ds4RightClickSection').classList.hidden);
 c.statusData={capabilities:{ds4_right_click:true},ds4_mouse:{right_click:false}};run('renderDS4RightClick()');
 let finish;c.post=()=>new Promise(resolve=>finish=resolve);
 $('ds4RightClickEnabled').checked=true;const pending=run('saveDS4RightClick()');
 c.selectedAdapterId='third';c.statusData.ds4_mouse={right_click:false};finish({settings:{right_click:true}});
 await pending;assert(!c.statusData.ds4_mouse.right_click);
 controller=null;run('renderDS4RightClick()');assert($('ds4RightClickSection').classList.hidden);
 console.log('right-click UI: default off, dirty refresh, save/failure, capability/read fallback and adapter-switch race passed');
})().catch(e=>{console.error(e);process.exitCode=1});
