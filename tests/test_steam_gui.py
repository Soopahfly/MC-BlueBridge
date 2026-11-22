"""Exercise the real Companion patch, optional reads, validation and HTTP routing."""
import ast
from contextlib import nullcontext
import importlib.util
import json
from pathlib import Path
import re
import sys
from types import SimpleNamespace
import urllib.parse

root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('steam_gui',root/'tools/companion_steam_features.py')
p=importlib.util.module_from_spec(spec);spec.loader.exec_module(p)
source=Path(sys.argv[1]).read_text(encoding='utf-8')
patched=p.patch(source);tree=ast.parse(p.daemon(patched))
assert p.patch(p.daemon(source))==p.daemon(patched)
assert p.patch(p.patch_base(source))==patched  # Upgrade the earlier Steam-only installer, too.
try:p.patch(patched)
except ValueError:pass
else:raise AssertionError('repeat installation must fail before writing')
helper=next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='save_steam_settings')
handler=next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='handle_client')
cls=next(n for n in tree.body if isinstance(n,ast.ClassDef) and n.name=='BlueBridgeManager')
status=next(n for n in cls.body if isinstance(n,ast.FunctionDef) and n.name=='status')
ns={};exec(compile(ast.Module(body=[helper,status],type_ignores=[]),'<steam-settings>','exec'),ns)
class Adapter:
    path='/dev/mock';info={};adapter_id='mock'
    def __init__(self,supported=True,fail=False):
        self.supported=supported;self.fail=fail;self.calls=[];self.lock=nullcontext()
    def request_json(self,cmd,*args):
        self.calls.append((cmd,args))
        if cmd=='HELLO':return {'capabilities':{'steam_features':self.supported}}
        if cmd.startswith('STEAM_') and cmd.endswith('_GET') and self.fail:raise ValueError('read failed')
        if cmd=='STEAM_AIM_SET':return {'restart_required':True}
        if cmd=='STEAM_FEATURES_GET':return dict(right_click=False,haptics=False,haptic_gain=50,led_brightness=100)
        return {}
    def request(self,*args):self.calls.append(args)

feedback=dict(right_click=True,haptics=True,haptic_gain=80,led_brightness=30)
pointer=dict(enabled=True,steam_aim=dict(enabled=True,sensitivity=125,activation=1,flags=3,pause_button=16))
a=Adapter();ns['save_steam_settings'](a,feedback)
assert ('STEAM_FEATURES_SET',1,1,80,30) in a.calls
a=Adapter();ns['save_steam_settings'](a,pointer,True)
assert ('STEAM_AIM_SET',(1,125,1,3,16,1)) in a.calls
for payload,is_pointer in [(None,False),([],False),({},False),(dict(feedback,haptics=1),False),
    (dict(feedback,led_brightness=101),False),(dict(feedback,haptic_gain=-1),False),
    (dict(pointer,enabled=False),True),(dict(pointer,steam_aim=dict(pointer['steam_aim'],sensitivity=True)),True),
    (dict(pointer,steam_aim=dict(pointer['steam_aim'],pause_button=18)),True)]:
    a=Adapter()
    try:ns['save_steam_settings'](a,payload,is_pointer)
    except ValueError:pass
    else:raise AssertionError(payload)
    assert not any('SET' in c[0] for c in a.calls)
for supported in (True,False):
    a=Adapter(supported);result=ns['status'](a)
    assert ('steam_features' in result)==supported
a=Adapter(fail=True);result=ns['status'](a)
assert result['detected'] and all(k+'_error' in result for k in ('steam_aim','steam_features','steam_diagnostics'))
for route,payload in [('steam-pointer',pointer),('steam-features',feedback)]:
    for method,supported,data,expected in [('GET',True,{},'405 Method Not Allowed'),('POST',False,payload,'400 Bad Request'),
        ('POST',True,[],'400 Bad Request'),('POST',True,payload,'200 OK')]:
        a=Adapter(supported);selected=[];replies=[]
        env=dict(ns,json=json,urllib=urllib,
            bluebridge_registry=SimpleNamespace(resolve=lambda id:selected.append(id) or a),
            _read_http_request=lambda sock:(method,'/api/bluebridge/'+route,{'x-bluebridge-adapter':'chosen'},json.dumps(data).encode()),
            _json_http=lambda sock,data,status='200 OK':replies.append((status,data)),response=lambda *args,**kw:kw)
        exec(compile(ast.Module(body=[handler],type_ignores=[]),'<route>','exec'),env)
        env['handle_client'](SimpleNamespace(close=lambda:None),'test','/remote/v1')
        assert selected==['chosen'] and replies[0][0]==expected
        assert any('SET' in c[0] for c in a.calls)==(expected=='200 OK')
html=re.search(r"BLUEBRIDGE_HTML = r'''(.*?)'''\.encode",patched,re.S)[1]
for id in ('steamPanel','steamTab','steamRightClick','steamHaptics','steamGyro','steamLED'):
    assert html.count('id="'+id+'"')==1
out=root/'build-host';out.mkdir(exist_ok=True)
(out/'steam-ui.js').write_text(p.JS,encoding='utf-8')
(out/'steam-full.js').write_text('\n'.join(re.findall(r'<script[^>]*>(.*?)</script>',html,re.S)),encoding='utf-8')
battery_helper=next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='save_steam_battery_settings')
exec(compile(ast.Module(body=[battery_helper],type_ignores=[]),'<battery-settings>','exec'),ns)
class BatteryAdapter(Adapter):
    def request_json(self,cmd,*args):
        if cmd=='HELLO':return {'capabilities':{'steam_battery':self.supported}}
        if cmd in ('STEAM_BATTERY_GET','STEAM_BATTERY_STATUS'):
            self.calls.append((cmd,args))
            if self.fail:raise ValueError('battery read failed')
            return dict(led=False,haptics=False,strength=35,threshold_mv=2200) if cmd.endswith('_GET') else dict(valid=False)
        return super().request_json(cmd,*args)
battery=dict(led=True,haptics=True,strength=35,threshold_mv=2150)
a=BatteryAdapter();ns['save_steam_battery_settings'](a,battery)
assert ('STEAM_BATTERY_SET',1,1,35,2150) in a.calls
for payload in [None,[],{},dict(battery,led=1),dict(battery,haptics='yes'),dict(battery,strength=0),
    dict(battery,strength=101),dict(battery,threshold_mv=True),dict(battery,threshold_mv=1799),dict(battery,threshold_mv=3001)]:
    a=BatteryAdapter()
    try:ns['save_steam_battery_settings'](a,payload)
    except ValueError:pass
    else:raise AssertionError(payload)
    assert not any('SET' in c[0] for c in a.calls)
for supported in (True,False):
    result=ns['status'](BatteryAdapter(supported));assert ('steam_battery' in result)==supported
result=ns['status'](BatteryAdapter(fail=True));assert result['detected'] and 'steam_battery_error' in result and 'steam_battery_status_error' in result
for route in ('steam-battery','steam-battery/test'):
    for method,supported,data,expected in [('GET',True,{},'405 Method Not Allowed'),('POST',False,battery,'400 Bad Request'),('POST',True,battery,'200 OK')]:
        a=BatteryAdapter(supported);selected=[];replies=[]
        env=dict(ns,json=json,urllib=urllib,
            bluebridge_registry=SimpleNamespace(resolve=lambda id:selected.append(id) or a),
            _read_http_request=lambda sock:(method,'/api/bluebridge/'+route,{'x-bluebridge-adapter':'chosen'},json.dumps(data).encode()),
            _json_http=lambda sock,data,status='200 OK':replies.append((status,data)),response=lambda *args,**kw:kw)
        exec(compile(ast.Module(body=[handler],type_ignores=[]),'<battery-route>','exec'),env)
        env['handle_client'](SimpleNamespace(close=lambda:None),'test','/remote/v1')
        assert selected==['chosen'] and replies[0][0]==expected
        command='STEAM_BATTERY_TEST' if route.endswith('/test') else 'STEAM_BATTERY_SET'
        assert any(c[0]==command for c in a.calls)==(expected=='200 OK')
(out/'steam-battery-ui.js').write_text(p.JS+p.BATTERY_JS,encoding='utf-8')
assert html.count('id="steamBatteryTest"')==1
print('Steam battery GUI: fresh install/upgrade, strict settings, optional reads, selected-adapter saves and test routing passed')
print('Steam GUI: patch/source parity, AST, strict validation, optional reads, selected adapter, methods and real HTTP saves passed')
