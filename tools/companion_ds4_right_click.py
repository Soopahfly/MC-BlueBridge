"""Add an optional DS4 right-click checkbox to Companion Remote 4.3.x."""
import argparse
import ast
import datetime
import os
from pathlib import Path
import re

MARKER = 'BLUEBRIDGE_DS4_RIGHT_CLICK_V1'
TARGETS = [Path('/media/fat/Scripts/companion_remote.sh'),
           Path('/media/fat/Scripts/.config/companion_remote/companion_remote_daemon')]
PANEL = '''<div id="ds4RightClickSection" class="hidden">
<label><input id="ds4RightClickEnabled" type="checkbox" onchange="ds4RightClickDirty=true"> Enable right-click on the right half of the DS4 touchpad</label>
<p class="subtle">Off: the whole pad clicks the left mouse button. On: left half = left-click; right half = right-click. A held click keeps its button until release. Mouse output must also be enabled.</p>
<button id="ds4RightClickSave" class="btn" onclick="saveDS4RightClick()" disabled>Save right-click option</button>
<p id="ds4RightClickStatus" class="subtle" role="status"></p>
</div>'''
JS = '''
// BLUEBRIDGE_DS4_RIGHT_CLICK_V1
let ds4RightClickDirty=false,ds4RightClickSaving=false,ds4RightClickSelection='';
function renderDS4RightClick(){
  const controller=selectedDS4();
  const key=JSON.stringify([selectedAdapterId,controller?.index,controller?.id]);
  if(key!==ds4RightClickSelection){ds4RightClickSelection=key;ds4RightClickDirty=false;}
  const supported=!!controller&&!!statusData.capabilities?.ds4_right_click;
  const data=statusData.ds4_mouse;
  $('ds4RightClickSection').classList.toggle('hidden',!supported);
  $('ds4RightClickEnabled').disabled=!supported||!data||ds4RightClickSaving;
  $('ds4RightClickSave').disabled=!supported||!data||ds4RightClickSaving;
  if(!supported)return;
  if(!data){$('ds4RightClickStatus').textContent='Unable to read right-click setting. Refresh to retry.';return;}
  if(!ds4RightClickDirty&&!ds4RightClickSaving)$('ds4RightClickEnabled').checked=!!data.right_click;
  $('ds4RightClickStatus').textContent=data.right_click?'Right-click is enabled.':'Right-click is off; the whole touchpad clicks left.';
}
async function saveDS4RightClick(){
  if(ds4RightClickSaving||!selectedDS4()||!statusData.ds4_mouse)return;
  const adapter=selectedAdapterId;
  ds4RightClickSaving=true;renderDS4RightClick();
  try{
    const result=await post('/api/bluebridge/ds4-mouse',{right_click:$('ds4RightClickEnabled').checked});
    if(selectedAdapterId!==adapter)return;
    statusData.ds4_mouse=result.settings;ds4RightClickDirty=false;
    toast('Right-click option saved. Release any held click to use the new setting.');
  }catch(e){toast(e.message,true);}finally{ds4RightClickSaving=false;renderDS4RightClick();}
}
saveDS4RightClick=guardAdapterAction(saveDS4RightClick);
'''
STATUS = '''            if status["capabilities"].get("ds4_right_click"):
                try:
                    status["ds4_mouse"] = self.request_json("DS4_MOUSE_GET")
                except Exception as right_click_error:
                    status["ds4_mouse_error"] = str(right_click_error)
'''
HELPER = '''# BLUEBRIDGE_DS4_RIGHT_CLICK_V1
def save_ds4_mouse_settings(bluebridge,payload):
    if not isinstance(payload,dict) or type(payload.get("right_click")) is not bool:
        raise ValueError("right_click must be a boolean")
    if not bluebridge.request_json("HELLO").get("capabilities",{}).get("ds4_right_click"):
        raise ValueError("This firmware does not support the right-click option")
    bluebridge.request("DS4_MOUSE_SET",int(payload["right_click"]))
    return bluebridge.request_json("DS4_MOUSE_GET")

'''
ROUTE = '''                if request_path == "/api/bluebridge/ds4-mouse":
                    if method != "POST":
                        _json_http(sock,response(False,"error","POST required"),"405 Method Not Allowed")
                        return
                    try:
                        payload=json.loads(request_body.decode("utf-8") or "{}")
                        settings=save_ds4_mouse_settings(bluebridge,payload)
                        _json_http(sock,response(True,"bluebridge","Right-click option saved",settings=settings))
                    except Exception as right_click_error:
                        _json_http(sock,response(False,"error",str(right_click_error)),"400 Bad Request")
                    return
'''

def daemon(text):
    return text.split("<<'PYEOF'\n",1)[1].split('\nPYEOF',1)[0] if "<<'PYEOF'\n" in text else text

def once(text,old,new):
    if text.count(old)!=1:raise ValueError('Unsupported Companion Remote source: '+old[:60])
    return text.replace(old,new,1)

def patch(text):
    if MARKER in text:raise ValueError('Right-click GUI option is already installed')
    match=re.search(r"BLUEBRIDGE_HTML = r'''(.*?)'''\.encode\(\"utf-8\"\)",text,re.S)
    if not match:raise ValueError('Cannot locate the BlueBridge web page')
    html=match[1]
    html=once(html,'<fieldset id="ds4GyroPanel"',PANEL+'<fieldset id="ds4GyroPanel"')
    html=once(html,'function renderAll(){','function renderAll(){renderDS4RightClick();')
    html=once(html,'</script>',JS+'</script>')
    text=text[:match.start(1)]+html+text[match.end(1):]
    anchor='            status["capabilities"] = hello.get("capabilities", {})\n'
    text=once(text,anchor,anchor+STATUS)
    text=once(text,'def handle_client(sock, address, path):',HELPER+'def handle_client(sock, address, path):')
    anchor='                if request_path == "/api/bluebridge/touch-mouse":\n'
    text=once(text,anchor,ROUTE+anchor)
    ast.parse(daemon(text))
    return text

def replace_file(path,data):
    temporary=path.with_name(path.name+'.right-click.tmp')
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
        backup=path.with_name(path.name+'.before-right-click-'+stamp)
        with backup.open('xb') as f:f.write(raw);f.flush();os.fsync(f.fileno())
        print('Backup:',backup)
    written=[]
    try:
        for path,raw,new in prepared:replace_file(path,new);written.append((path,raw));print('Patched:',path)
    except Exception:
        for path,raw in reversed(written):replace_file(path,raw)
        raise
    print('Rollback: python3 /tmp/add_ds4_right_click.py --restore '+stamp)

def restore(stamp):
    if not re.fullmatch(r'\d{8}-\d{6}',stamp):raise ValueError('Invalid backup timestamp')
    prepared=[]
    for path in TARGETS:
        if MARKER not in path.read_text(encoding='utf-8'):raise ValueError('Target changed; refusing to overwrite '+str(path))
        raw=path.with_name(path.name+'.before-right-click-'+stamp).read_bytes()
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
        args.preview.with_name('companion_remote.right-click.sh').write_text(result,encoding='utf-8',newline='\n')
        print('Preview syntax validated; nothing installed')
    else:parser.error('Choose --install, --restore TIMESTAMP or --preview FILE')
