"""Verify the optional checkbox/API patch against pristine Companion Remote."""
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
spec=importlib.util.spec_from_file_location('right_click',root/'tools/companion_ds4_right_click.py')
p=importlib.util.module_from_spec(spec);spec.loader.exec_module(p)
source=Path(sys.argv[1]).read_text(encoding='utf-8')
patched=p.patch(source);tree=ast.parse(p.daemon(patched))
assert p.patch(p.daemon(source))==p.daemon(patched)
try:p.patch(patched)
except ValueError:pass
else:raise AssertionError('must reject repeat install')
helper=next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='save_ds4_mouse_settings')
handler=next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='handle_client')
cls=next(n for n in tree.body if isinstance(n,ast.ClassDef) and n.name=='BlueBridgeManager')
status=next(n for n in cls.body if isinstance(n,ast.FunctionDef) and n.name=='status')
ns={}
exec(compile(ast.Module(body=[helper,status],type_ignores=[]),'<settings>','exec'),ns)
class Adapter:
    path='/dev/mock';info={};adapter_id='mock'
    def __init__(self,supported=True,fail=False):self.calls=[];self.supported=supported;self.fail=fail;self.lock=nullcontext()
    def request_json(self,cmd,*args):
        self.calls.append((cmd,args))
        if cmd=='HELLO':return {'capabilities':{'ds4_right_click':self.supported}}
        if cmd=='DS4_MOUSE_GET' and self.fail:raise ValueError('read failed')
        return {'right_click':False} if cmd=='DS4_MOUSE_GET' else {}
    def request(self,*args):self.calls.append(args)

for enabled in (True,False):
    a=Adapter();ns['save_ds4_mouse_settings'](a,dict(right_click=enabled))
    assert ('DS4_MOUSE_SET',int(enabled)) in a.calls
for payload in [None,[],{},dict(right_click=1),dict(right_click='true')]:
    a=Adapter()
    try:ns['save_ds4_mouse_settings'](a,payload)
    except ValueError:pass
    else:raise AssertionError(payload)
    assert not a.calls
for supported in (True,False):
    a=Adapter(supported);result=ns['status'](a)
    assert ('ds4_mouse' in result)==supported
a=Adapter(fail=True);assert 'ds4_mouse_error' in ns['status'](a)
for method,supported,payload,expected in [('GET',True,{},'405 Method Not Allowed'),
    ('POST',False,dict(right_click=True),'400 Bad Request'),('POST',True,[], '400 Bad Request'),
    ('POST',True,dict(right_click=True),'200 OK')]:
    a=Adapter(supported);replies=[];selected=[]
    env=dict(ns,json=json,urllib=urllib,
        bluebridge_registry=SimpleNamespace(resolve=lambda id:selected.append(id) or a),
        _read_http_request=lambda sock:(method,'/api/bluebridge/ds4-mouse',{'x-bluebridge-adapter':'chosen'},json.dumps(payload).encode()),
        _json_http=lambda sock,data,status='200 OK':replies.append(status),
        response=lambda *args,**kw:kw)
    exec(compile(ast.Module(body=[handler],type_ignores=[]),'<route>','exec'),env)
    env['handle_client'](SimpleNamespace(close=lambda:None),'test','/remote/v1')
    assert selected==['chosen'] and replies==[expected]
    assert any(c[0]=='DS4_MOUSE_SET' for c in a.calls)==(expected=='200 OK')
html=re.search(r"BLUEBRIDGE_HTML = r'''(.*?)'''\.encode",patched,re.S)[1]
assert html.count('id="ds4RightClickEnabled"')==1
out=root/'build-host';out.mkdir(exist_ok=True)
(out/'right-click-ui.js').write_text(p.JS,encoding='utf-8')
(out/'right-click-full.js').write_text('\n'.join(re.findall(r'<script[^>]*>(.*?)</script>',html,re.S)),encoding='utf-8')
print('right-click GUI: patch/source parity, syntax, boolean validation, capability/read failure, selected-adapter HTTP methods and saves passed')
