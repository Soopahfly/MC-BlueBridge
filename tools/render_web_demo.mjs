// Generate an explicitly simulated, self-contained copy of the actual config UI.
import {readFileSync,writeFileSync,mkdirSync} from 'node:fs';
import {fileURLToPath} from 'node:url';
const source=readFileSync(new URL('oled-config.html',import.meta.url),'utf8');
const mock=String.raw`
<script>
(()=>{
let input='trackball',activeInput='trackball',oled=true,activeOled=true,queue=[],waiting,stopped=false,mouse=false,activeMouse=false;
const effects=[{enabled:false,fire:0},{enabled:true,fire:0}];
function enqueue(text){const result={value:new TextEncoder().encode(text),done:false};if(waiting){const resolve=waiting;waiting=null;resolve(result);}else queue.push(result);}
const reader={read:()=>queue.length?Promise.resolve(queue.shift()):stopped?Promise.resolve({done:true}):new Promise(resolve=>{waiting=resolve;}),releaseLock(){},async cancel(){stopped=true;if(waiting){waiting({done:true});waiting=null;}}};
const writer={releaseLock(){},async write(bytes){
  const [,id,command,...args]=new TextDecoder().decode(bytes).trim().split('|');
  let data,restart=false;
  if(command==='HELLO')data={capabilities:{profile_oled:true,panel_configuration:true,oled_toggle:true,touch_mouse:true,steam_ble:true}};
  else if(command==='TOUCH_MOUSE_GET')data={enabled:mouse,active:activeMouse,available:true};
  else if(command==='TOUCH_MOUSE_SET'){mouse=args[0]==='1';restart=mouse!==activeMouse;data={restart_required:restart};}
  else if(command==='PANEL_GET')data={input,active_input:activeInput,oled_enabled:oled,active_oled_enabled:activeOled,panel_compiled:true,trackball_ready:activeInput==='trackball'};
  else if(command==='PANEL_SET'){input=args[0];oled=args[1]==='1';restart=input!==activeInput||oled!==activeOled;data={input,restart_required:restart};}
  else if(command==='CONTROLLERS')data={selected_index:0,controllers:[{index:0,name:'DualSense (simulated)'}]};
  else if(command==='CONTROLLER_SELECT')data='selected';
  else if(command==='PROFILES')data={active:0,profiles:[{index:0,name:'Default'},{index:1,name:'Arcade'}],labels:['Cross','Circle','Square','Triangle','L1','R1','L2','R2','Create','Options','L3','R3','PS','Touchpad','Mute','Extra','L4','R4','L5','R5','P1','P2','P3','P4','Aux 1','Aux 2']};
  else if(command==='PROFILE_GET'){const p=effects[Number(args[0])];data={id:100+Number(args[0]),oled_pew_enabled:p.enabled,oled_fire_button:p.fire};}
  else if(command==='PROFILE_OLED'){effects[Number(args[0])]={enabled:args[1]==='1',fire:Number(args[2])};data='saved';}
  else {enqueue('BB1|'+id+'|ERR|Unsupported demo command\n');return;}
  enqueue('BB1|'+id+'|OK|'+JSON.stringify(data)+'\n');
  if(restart)setTimeout(()=>reader.cancel(),200);
}};
const port={readable:{getReader:()=>reader},writable:{getWriter:()=>writer},async open(){queue=[];stopped=false;activeInput=input;activeOled=oled;activeMouse=mouse;},async close(){}};
Object.defineProperty(navigator,'serial',{configurable:true,value:{requestPort:async()=>port}});
window.addEventListener('DOMContentLoaded',()=>{document.getElementById('connect').click();});
})();
</script>
`;
const banner='<p style="padding:12px;border:1px solid #92d4ff;color:#e6eef5">SIMULATED ADAPTER — interactive demonstration only. This page never connects to real USB hardware.</p>';
const output=source.replace('<h1>BlueBridge OLED</h1>',banner+'<h1>BlueBridge OLED</h1>').replace('<script type="module">',mock+'<script type="module">');
const folder=new URL('../docs/demos/',import.meta.url);mkdirSync(folder,{recursive:true});
writeFileSync(new URL('oled-config-demo.html',folder),output);
console.log('Generated '+fileURLToPath(new URL('oled-config-demo.html',folder)));
