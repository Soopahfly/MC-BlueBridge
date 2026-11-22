"""Add original Steam Controller settings to Companion Remote's BlueBridge page."""
import argparse
import ast
import datetime
import os
from pathlib import Path
import re

MARKER = 'BLUEBRIDGE_STEAM_FEATURES_V1'
TARGETS = [Path('/media/fat/Scripts/companion_remote.sh'),
           Path('/media/fat/Scripts/.config/companion_remote/companion_remote_daemon')]
PANEL = '''<div id="steamPanel" class="tabpage hidden"><div class="surface">
<h3>Steam Controller v1</h3>
<p class="subtle">Original 2015 controller with Bluetooth LE firmware. Settings apply to this adapter. Mouse output requires single-controller MiSTer mode. Exit Controller Tester to use the mouse.</p>
<fieldset id="steamPointerFields">
<label><input id="steamMouse" type="checkbox"> Use the right pad as a USB mouse</label>
<label><input id="steamGyro" type="checkbox"> Enable gyro mouse (experimental)</label>
<div class="field-grid"><label>Sensitivity (%)<input id="steamSensitivity" type="number" min="1" max="200" step="1"></label>
<label>Move the pointer<select id="steamActivation"><option value="0">Always</option><option value="1">While touching the right pad</option><option value="2">While holding L2</option></select></label>
<label>Hold to pause and zero drift<select id="steamPause"></select></label></div>
<label><input id="steamInvertX" type="checkbox"> Invert horizontal movement</label>
<label><input id="steamInvertY" type="checkbox"> Invert vertical movement</label>
<label><input id="steamFire" type="checkbox"> Use R2 for left mouse click</label>
<p class="subtle">Hold the pause button with the controller still for at least half a second to zero drift. R2 mouse click replaces gamepad R2. Enabling USB mouse output also enables DS4 touchpad output if a DS4 is connected.</p>
</fieldset>
<button id="steamPointerSave" class="btn" onclick="saveSteamPointer()">Save mouse settings</button>
<p id="steamPointerStatus" class="subtle" role="status"></p>
<fieldset id="steamFeedbackFields">
<label><input id="steamRightClick" type="checkbox"> Use left pad press for right mouse click</label>
<p class="subtle">Off by default. Right pad press clicks left. When enabled, left pad press clicks right and replaces its gamepad click while mouse output is active.</p>
<label><input id="steamHaptics" type="checkbox"> Enable haptic rumble (experimental)</label>
<div class="field-grid"><label>Haptic strength (%)<input id="steamGain" type="number" min="0" max="100" step="1"></label>
<label>White LED brightness (%)<input id="steamLED" type="number" min="0" max="100" step="1"></label></div>
<p class="subtle">Game rumble drives the left and right haptic pads independently where MiSTer supplies separate channels. The Steam logo is a white LED; colour themes are unavailable.</p>
</fieldset>
<button id="steamFeedbackSave" class="btn" onclick="saveSteamFeedback()">Save click and feedback settings</button>
<p id="steamFeedbackStatus" class="subtle" role="status"></p>
</div></div>'''

JS = '''
// BLUEBRIDGE_STEAM_FEATURES_V1
let steamPointerDirty=false,steamFeedbackDirty=false,steamSaving=false,steamSelection='';
function selectedSteam(){
  if(!statusData.detected)return null;
  const selected=selectedController();
  const c=(offlineManagement||selected?.connected)?selected:controllerByIndex(controllersData.connected_index);
  return Number(c?.kind)===18?c:null;
}
function renderSteam(){
  const c=selectedSteam(),key=c?JSON.stringify([selectedAdapterId,c.index,c.id]):'';
  if(key!==steamSelection){steamSelection=key;steamPointerDirty=steamFeedbackDirty=false;}
  const supported=!!c&&!!statusData.capabilities?.steam_features;
  setDS4TabVisible('steam',supported);
  const aim=statusData.steam_aim,mouse=statusData.touch_mouse,f=statusData.steam_features;
  $('steamPointerFields').disabled=!supported||!aim||!mouse||steamSaving;
  $('steamFeedbackFields').disabled=!supported||!f||steamSaving;
  $('steamPointerSave').disabled=$('steamPointerFields').disabled;
  $('steamFeedbackSave').disabled=$('steamFeedbackFields').disabled;
  if(!supported)return;
  if(aim&&mouse&&!steamPointerDirty&&!steamSaving){
    $('steamMouse').checked=!!mouse.enabled;$('steamGyro').checked=!!aim.enabled;
    $('steamSensitivity').value=String(aim.sensitivity);$('steamActivation').value=String(aim.activation);
    $('steamInvertX').checked=!!(aim.flags&1);$('steamInvertY').checked=!!(aim.flags&2);$('steamFire').checked=!!(aim.flags&4);
    const labels=['A','B','X','Y','LB','RB','L2','R2','Back','Start','Stick click','Right pad press','Steam','Left pad press',null,null,'Left grip','Right grip'];
    $('steamPause').replaceChildren(new Option('None',255),...labels.flatMap((n,i)=>n?[new Option(n,i)]:[]));
    $('steamPause').value=String(aim.pause_button);
  }
  $('steamPointerStatus').textContent=!aim||!mouse?'Unable to read mouse settings. Refresh to retry.':
    !mouse.available||statusData.multi_controller||Number(statusData.output_mode)!==1?'Mouse output requires single-controller MiSTer mode. Your saved choice is retained.':
    aim.restart_required||mouse.enabled!==mouse.active?'Settings saved. Waiting for the adapter to restart.':mouse.active?'Mouse output is active.':'Mouse output is off.';
  if(f&&!steamFeedbackDirty&&!steamSaving){
    $('steamRightClick').checked=!!f.right_click;$('steamHaptics').checked=!!f.haptics;
    $('steamGain').value=String(f.haptic_gain);$('steamLED').value=String(f.led_brightness);
  }
  const diagnostic=statusData.steam_diagnostics;
  $('steamFeedbackStatus').textContent=!f?'Unable to read feedback settings. Refresh to retry.':
    diagnostic?.steam_transport&&diagnostic.output_failed?'Feedback was rejected or timed out. Controller input remains available. Reconnect to retry.':'Click and feedback changes apply without restarting.';
}
function steamInteger(id,min,max){const n=Number($(id).value);if(!Number.isInteger(n)||n<min||n>max)throw Error('Choose a whole number from '+min+' to '+max+'.');return n;}
async function saveSteamPointer(){
  if(steamSaving||!selectedSteam()||!statusData.steam_aim||!statusData.touch_mouse)return;
  const adapter=selectedAdapterId;
  try{
  const payload={enabled:$('steamMouse').checked,steam_aim:{enabled:$('steamGyro').checked,
    sensitivity:steamInteger('steamSensitivity',1,200),activation:steamInteger('steamActivation',0,2),
    flags:($('steamInvertX').checked?1:0)|($('steamInvertY').checked?2:0)|($('steamFire').checked?4:0),pause_button:steamInteger('steamPause',0,255)}};
  if(payload.steam_aim.enabled&&!payload.enabled)throw Error('Enable USB mouse output to use gyro.');
  steamSaving=true;renderSteam();
  const r=await post('/api/bluebridge/steam-pointer',payload);if(selectedAdapterId!==adapter)return;
    statusData.steam_aim={...payload.steam_aim,restart_required:!!r.mouse?.restart_required};
    statusData.touch_mouse.enabled=payload.enabled;steamPointerDirty=false;
    toast(r.mouse?.restart_required?'Steam mouse settings saved. Adapter restarting…':'Steam mouse settings saved.');
    setTimeout(()=>refresh(),1500);
  }catch(e){toast(e.message,true);}finally{steamSaving=false;renderSteam();}
}
async function saveSteamFeedback(){
  if(steamSaving||!selectedSteam()||!statusData.steam_features)return;
  const adapter=selectedAdapterId;
  try{
  const payload={right_click:$('steamRightClick').checked,haptics:$('steamHaptics').checked,
    haptic_gain:steamInteger('steamGain',0,100),led_brightness:steamInteger('steamLED',0,100)};
  steamSaving=true;renderSteam();
  const r=await post('/api/bluebridge/steam-features',payload);if(selectedAdapterId!==adapter)return;
    statusData.steam_features=r.settings;steamFeedbackDirty=false;toast('Steam click and feedback settings saved.');
  }catch(e){toast(e.message,true);}finally{steamSaving=false;renderSteam();}
}
for(const event of ['input','change']){
  $('steamPointerFields').addEventListener(event,()=>steamPointerDirty=true);
  $('steamFeedbackFields').addEventListener(event,()=>steamFeedbackDirty=true);
}
saveSteamPointer=guardAdapterAction(saveSteamPointer);
saveSteamFeedback=guardAdapterAction(saveSteamFeedback);
'''

STATUS = '''            if status["capabilities"].get("steam_features"):
                for key,command in [("steam_aim","STEAM_AIM_GET"),("steam_features","STEAM_FEATURES_GET"),("steam_diagnostics","STEAM_GET")]:
                    try:
                        status[key] = self.request_json(command)
                    except Exception as steam_error:
                        status[key+"_error"] = str(steam_error)
'''

HELPER = '''# BLUEBRIDGE_STEAM_FEATURES_V1
def save_steam_settings(bluebridge,payload,pointer=False):
    if not isinstance(payload,dict):raise ValueError("Invalid Steam Controller settings")
    if not bluebridge.request_json("HELLO").get("capabilities",{}).get("steam_features"):
        raise ValueError("This firmware does not support Steam Controller settings")
    if pointer:
        aim=payload.get("steam_aim")
        if type(payload.get("enabled")) is not bool or not isinstance(aim,dict) or type(aim.get("enabled")) is not bool:
            raise ValueError("Invalid mouse settings")
        for name,low,high in [("sensitivity",1,200),("activation",0,2),("flags",0,7),("pause_button",0,255)]:
            value=aim.get(name)
            if type(value) is not int or not low<=value<=high:raise ValueError("Invalid Steam gyro "+name)
        if aim["pause_button"]>17 and aim["pause_button"]!=255:raise ValueError("Invalid Steam pause button")
        if aim["enabled"] and not payload["enabled"]:raise ValueError("Enable USB mouse output to use gyro")
        result=bluebridge.request_json("STEAM_AIM_SET",int(aim["enabled"]),aim["sensitivity"],aim["activation"],aim["flags"],aim["pause_button"],int(payload["enabled"]))
        return result
    for name in ["right_click","haptics"]:
        if type(payload.get(name)) is not bool:raise ValueError(name+" must be a boolean")
    for name in ["haptic_gain","led_brightness"]:
        if type(payload.get(name)) is not int or not 0<=payload[name]<=100:raise ValueError(name+" must be from 0 to 100")
    bluebridge.request("STEAM_FEATURES_SET",int(payload["right_click"]),int(payload["haptics"]),payload["haptic_gain"],payload["led_brightness"])
    return bluebridge.request_json("STEAM_FEATURES_GET")

'''

ROUTE = '''                if request_path in ("/api/bluebridge/steam-pointer","/api/bluebridge/steam-features"):
                    if method != "POST":
                        _json_http(sock,response(False,"error","POST required"),"405 Method Not Allowed")
                        return
                    try:
                        payload=json.loads(request_body.decode("utf-8") or "{}")
                        pointer=request_path.endswith("/steam-pointer")
                        result=save_steam_settings(bluebridge,payload,pointer)
                        _json_http(sock,response(True,"bluebridge","Steam settings saved",**{"mouse" if pointer else "settings":result}))
                    except Exception as steam_error:
                        _json_http(sock,response(False,"error",str(steam_error)),"400 Bad Request")
                    return
'''

def daemon(text):
    return text.split("<<'PYEOF'\n",1)[1].split('\nPYEOF',1)[0] if "<<'PYEOF'\n" in text else text

def once(text,old,new):
    if text.count(old)!=1:raise ValueError('Unsupported Companion Remote source: '+old[:60])
    return text.replace(old,new,1)

def patch_base(text):
    if MARKER in text:raise ValueError('Steam Controller settings are already installed')
    match=re.search(r"BLUEBRIDGE_HTML = r'''(.*?)'''\.encode\(\"utf-8\"\)",text,re.S)
    if not match:raise ValueError('Cannot locate the BlueBridge web page')
    html=match[1]
    anchor='<button id="touchMouseTab"'
    html=once(html,anchor,'<button id="steamTab" class="tab hidden" data-tab="steamPanel">Steam pads &amp; gyro</button>'+anchor)
    html=once(html,'<div id="touchMousePanel"',PANEL+'<div id="touchMousePanel"')
    html=once(html,'function renderAll(){','function renderAll(){renderSteam();')
    html=once(html,'</script>',JS+'</script>')
    text=text[:match.start(1)]+html+text[match.end(1):]
    anchor='            status["capabilities"] = hello.get("capabilities", {})\n'
    text=once(text,anchor,anchor+STATUS)
    text=once(text,'def handle_client(sock, address, path):',HELPER+'def handle_client(sock, address, path):')
    anchor='                if request_path == "/api/bluebridge/touch-mouse":\n'
    text=once(text,anchor,ROUTE+anchor)
    ast.parse(daemon(text))
    return text

BATTERY_MARKER='BLUEBRIDGE_STEAM_BATTERY_V1'
BATTERY_PANEL='''<div id="steamBatterySection" class="hidden">
<h3>Battery warning (experimental)</h3>
<p id="steamBatteryReading" class="subtle" role="status"></p>
<fieldset id="steamBatteryFields">
<label><input id="steamBatteryLED" type="checkbox"> Double-blink the white LED when battery is low</label>
<label><input id="steamBatteryHaptics" type="checkbox"> Play a haptic battery warning</label>
<div class="field-grid"><label>Warning strength (%)<input id="steamBatteryStrength" type="number" min="1" max="100" step="1"></label>
<label>Warn below (V, both AA batteries)<input id="steamBatteryVoltage" type="number" min="1.8" max="3" step="0.05"></label></div>
<p class="subtle">Left tap, right tap, then a longer pulse on both pads. Warns once, then at most every five minutes while low. Independent of game rumble. LED double-blinks every five seconds, including when normal brightness is off.</p>
<p class="subtle">These warnings default off. The initial 2.20 V threshold is experimental: check it against your AA batteries. Voltage is shown without estimating a percentage. Missing or stale readings suppress automatic warnings.</p>
</fieldset>
<button id="steamBatterySave" class="btn" onclick="saveSteamBattery()">Save battery warnings</button>
<button id="steamBatteryTest" class="btn" onclick="testSteamBattery()">Test LED and haptic warning</button>
<p id="steamBatteryStatus" class="subtle" role="status"></p>
</div>'''
BATTERY_JS='''
// BLUEBRIDGE_STEAM_BATTERY_V1
let steamBatteryDirty=false,steamBatterySaving=false,steamBatteryTesting=false,steamBatterySelection='';
function renderSteamBattery(){
  const c=selectedSteam(),key=c?JSON.stringify([selectedAdapterId,c.index,c.id]):'';
  if(key!==steamBatterySelection){steamBatterySelection=key;steamBatteryDirty=false;}
  const supported=!!c&&!!statusData.capabilities?.steam_battery;
  $('steamBatterySection').classList.toggle('hidden',!supported);
  const settings=statusData.steam_battery,b=statusData.steam_battery_status;
  const connected=!!c?.connected&&Number(c.index)===Number(b?.controller_index);
  $('steamBatteryFields').disabled=!supported||!settings||steamBatterySaving||steamBatteryTesting;
  $('steamBatterySave').disabled=$('steamBatteryFields').disabled;
  $('steamBatteryTest').disabled=!supported||!connected||!b?.test_available||steamBatterySaving||steamBatteryTesting;
  if(!supported)return;
  if(settings&&!steamBatteryDirty&&!steamBatterySaving){
    $('steamBatteryLED').checked=!!settings.led;$('steamBatteryHaptics').checked=!!settings.haptics;
    $('steamBatteryStrength').value=String(settings.strength);$('steamBatteryVoltage').value=(settings.threshold_mv/1000).toFixed(2);
  }
  $('steamBatteryReading').textContent=connected&&b?.valid?'Battery: '+(b.millivolts/1000).toFixed(2)+' V'+(b.low?' - low':''):
    connected?'Battery voltage unavailable or stale. Automatic warnings are suppressed.':'Connect this Steam Controller to read battery voltage or test the warning.';
  $('steamBatteryStatus').textContent=!settings?'Unable to read warning settings. Refresh to retry.':'Warnings save without restarting. Test plays both outputs without changing your saved choices.';
}
async function saveSteamBattery(){
  if(steamBatterySaving||steamBatteryTesting||!selectedSteam()||!statusData.steam_battery)return;
  const adapter=selectedAdapterId;
  try{
    const voltage=Number($('steamBatteryVoltage').value),mv=Math.round(voltage*1000);
    if(!Number.isFinite(voltage)||mv<1800||mv>3000||Math.abs(voltage*1000-mv)>0.00001)throw Error('Choose a voltage from 1.80 to 3.00 V.');
    const payload={led:$('steamBatteryLED').checked,haptics:$('steamBatteryHaptics').checked,
      strength:steamInteger('steamBatteryStrength',1,100),threshold_mv:mv};
    steamBatterySaving=true;renderSteamBattery();
    const r=await post('/api/bluebridge/steam-battery',payload);if(selectedAdapterId!==adapter)return;
    statusData.steam_battery=r.settings;steamBatteryDirty=false;toast('Steam battery warnings saved.');
  }catch(e){toast(e.message,true);}finally{steamBatterySaving=false;renderSteamBattery();}
}
async function testSteamBattery(){
  if(steamBatterySaving||steamBatteryTesting||$('steamBatteryTest').disabled)return;
  steamBatteryTesting=true;renderSteamBattery();
  try{await post('/api/bluebridge/steam-battery/test',{});toast('Battery warning test queued. Uses saved warning strength.');}
  catch(e){toast(e.message,true);}finally{steamBatteryTesting=false;renderSteamBattery();}
}
for(const event of ['input','change'])$('steamBatteryFields').addEventListener(event,()=>steamBatteryDirty=true);
saveSteamBattery=guardAdapterAction(saveSteamBattery);
testSteamBattery=guardAdapterAction(testSteamBattery);
'''
BATTERY_STATUS='''            if status["capabilities"].get("steam_battery"):
                for key,command in [("steam_battery","STEAM_BATTERY_GET"),("steam_battery_status","STEAM_BATTERY_STATUS")]:
                    try:
                        status[key]=self.request_json(command)
                    except Exception as battery_error:
                        status[key+"_error"]=str(battery_error)
'''
BATTERY_HELPER='''# BLUEBRIDGE_STEAM_BATTERY_V1
def save_steam_battery_settings(bluebridge,payload):
    if not isinstance(payload,dict):raise ValueError("Invalid battery warning settings")
    if not bluebridge.request_json("HELLO").get("capabilities",{}).get("steam_battery"):
        raise ValueError("This firmware does not support Steam battery warnings")
    for name in ["led","haptics"]:
        if type(payload.get(name)) is not bool:raise ValueError(name+" must be a boolean")
    for name,low,high in [("strength",1,100),("threshold_mv",1800,3000)]:
        value=payload.get(name)
        if type(value) is not int or not low<=value<=high:raise ValueError("Invalid "+name)
    bluebridge.request("STEAM_BATTERY_SET",int(payload["led"]),int(payload["haptics"]),payload["strength"],payload["threshold_mv"])
    return bluebridge.request_json("STEAM_BATTERY_GET")

'''
BATTERY_ROUTE='''                if request_path in ("/api/bluebridge/steam-battery","/api/bluebridge/steam-battery/test"):
                    if method != "POST":
                        _json_http(sock,response(False,"error","POST required"),"405 Method Not Allowed")
                        return
                    try:
                        if request_path.endswith("/test"):
                            if not bluebridge.request_json("HELLO").get("capabilities",{}).get("steam_battery"):
                                raise ValueError("This firmware does not support Steam battery warnings")
                            bluebridge.request("STEAM_BATTERY_TEST")
                            _json_http(sock,response(True,"bluebridge","Warning test queued"))
                        else:
                            payload=json.loads(request_body.decode("utf-8") or "{}")
                            settings=save_steam_battery_settings(bluebridge,payload)
                            _json_http(sock,response(True,"bluebridge","Battery warnings saved",settings=settings))
                    except Exception as battery_error:
                        _json_http(sock,response(False,"error",str(battery_error)),"400 Bad Request")
                    return
'''

def patch(text):
    if BATTERY_MARKER in text:raise ValueError('Steam battery warnings are already installed')
    if MARKER not in text:text=patch_base(text)
    anchor='<p id="steamFeedbackStatus" class="subtle" role="status"></p>'
    text=once(text,anchor,anchor+BATTERY_PANEL)
    text=once(text,'function renderAll(){','function renderAll(){renderSteamBattery();')
    match=re.search(r"BLUEBRIDGE_HTML = r'''(.*?)'''\.encode",text,re.S)
    if not match:raise ValueError('Cannot locate BlueBridge web page')
    html=match[1];html=once(html,'</script>',BATTERY_JS+'</script>')
    text=text[:match.start(1)]+html+text[match.end(1):]
    anchor='            status["capabilities"] = hello.get("capabilities", {})\n'
    text=once(text,anchor,anchor+BATTERY_STATUS)
    text=once(text,'def handle_client(sock, address, path):',BATTERY_HELPER+'def handle_client(sock, address, path):')
    anchor='                if request_path == "/api/bluebridge/touch-mouse":\n'
    text=once(text,anchor,BATTERY_ROUTE+anchor)
    ast.parse(daemon(text));return text

def replace_file(path,data):
    temporary=path.with_name(path.name+'.steam.tmp')
    with temporary.open('wb') as f:f.write(data);f.flush();os.fsync(f.fileno())
    temporary.chmod(path.stat().st_mode)
    os.replace(str(temporary),str(path))

def install():
    prepared=[]
    for path in TARGETS:
        raw=path.read_bytes();text=raw.decode('utf-8').replace('\r\n','\n')
        prepared.append((path,raw,patch(text).encode('utf-8')))
    stamp=datetime.datetime.now().strftime('%Y%m%d-%H%M%S')
    for path,raw,new in prepared:
        backup=path.with_name(path.name+'.before-steam-'+stamp)
        with backup.open('xb') as f:f.write(raw);f.flush();os.fsync(f.fileno())
        print('Backup:',backup)
    written=[]
    try:
        for path,raw,new in prepared:replace_file(path,new);written.append((path,raw));print('Patched:',path)
    except Exception:
        for path,raw in reversed(written):replace_file(path,raw)
        raise
    print('Rollback: python3 /tmp/add_steam_features.py --restore '+stamp)

def restore(stamp):
    if not re.fullmatch(r'\d{8}-\d{6}',stamp):raise ValueError('Invalid backup timestamp')
    prepared=[]
    for path in TARGETS:
        if BATTERY_MARKER not in path.read_text(encoding='utf-8'):raise ValueError('Target changed; refusing to overwrite '+str(path))
        raw=path.with_name(path.name+'.before-steam-'+stamp).read_bytes()
        ast.parse(daemon(raw.decode('utf-8').replace('\r\n','\n')));prepared.append((path,raw))
    for path,raw in prepared:replace_file(path,raw);print('Restored:',path)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--install',action='store_true')
    parser.add_argument('--restore')
    parser.add_argument('--preview',type=Path)
    args=parser.parse_args()
    if args.install:install()
    elif args.restore:restore(args.restore)
    elif args.preview:
        result=patch(args.preview.read_text(encoding='utf-8'))
        args.preview.with_name('companion_remote.steam.sh').write_text(result,encoding='utf-8',newline='\n')
        print('Preview syntax validated; nothing installed')
    else:parser.error('Choose --install, --restore TIMESTAMP or --preview FILE')
