"""Generate documentation PNG/GIF assets from native firmware renderers.

Requires a native C compiler (CC or gcc), plus Pillow. No Pico is connected.
Pillow can be installed locally: python -m pip install --target .build-tools/python pillow
"""
from pathlib import Path
import os
import shutil
import subprocess
import sys

root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'.build-tools/python'))
from PIL import Image,ImageDraw,ImageFont

cc=os.environ.get('CC') or shutil.which('gcc')
if not cc and Path('C:/msys64/mingw64/bin/gcc.exe').exists():
    cc='C:/msys64/mingw64/bin/gcc.exe'
if not cc: raise SystemExit('Set CC to a native compiler or put gcc on PATH.')
env=dict(os.environ);env['PATH']=str(Path(cc).parent)+os.pathsep+env.get('PATH','')
frames=root/'build-host/demo-frames';frames.mkdir(parents=True,exist_ok=True)
assets=root/'docs/images';assets.mkdir(parents=True,exist_ok=True)
exe=root/'build-host'/('render-demos.exe' if os.name=='nt' else 'render-demos')
subprocess.run([cc,'-std=c11','-Wall','-Wextra','-Werror','-Itests/stubs','-Isrc',
    '-DBB_OLED_I2C=0','-DBB_OLED_SDA=4','-DBB_OLED_SCL=5','-DBB_OLED_ADDRESS=0x3c',
    'tools/render_oled_demos.c','src/oled_display.c','src/oled_gamepad.c','-o',str(exe)],cwd=root,env=env,check=True)
subprocess.run([str(exe)],cwd=root,env=env,check=True)
palette=[2,6,9,198,244,255]+[0]*762
def panel(name,scale=4):
    # PBM black pixels are OLED lit pixels; invert that convention for display.
    source=Image.open(frames/(name+'.pbm')).convert('1')
    image=Image.new('P',source.size);image.putpalette(palette)
    image.putdata([1 if p==0 else 0 for p in source.convert('L').tobytes()])
    return image.resize((128*scale,64*scale),Image.Resampling.NEAREST)
for name in ['playstation','xbox','nintendo','status','menu']:
    panel(name).save(assets/(name+'.png'))
font=ImageFont.load_default(size=20)
strip=Image.new('RGB',(560,3*310),(17,24,32));draw=ImageDraw.Draw(strip)
for n,(name,label) in enumerate([('playstation','PlayStation'),('xbox','Xbox'),('nintendo','Nintendo / 8BitDo')]):
    draw.text((24,n*310+12),label,font=font,fill=(230,238,245))
    strip.paste(panel(name).convert('RGB'),(24,n*310+46))
strip.save(assets/'controller-layouts.png')
sequence=[panel(f'input-{n:02}') for n in range(12)]
sequence[0].save(assets/'live-input-demo.gif',save_all=True,append_images=sequence[1:],duration=[700]+[300]*10+[700],loop=0,optimize=False,disposal=2)
sequence=[panel('pew-status')]+[panel(f'pew-{n}') for n in range(4)]+[panel('pew-status')]
sequence[0].save(assets/'pew-demo.gif',save_all=True,append_images=sequence[1:],duration=[1200,100,100,100,100,1200],loop=0,optimize=False,disposal=2)
print('Generated firmware-derived PNG screenshots and GIF demonstrations in docs/images/.')
