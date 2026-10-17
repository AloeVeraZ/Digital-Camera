"""Generate synthetic SD-card fixtures; never reads or changes a real card."""
import struct
import sys
from pathlib import Path
from PIL import Image, ImageDraw
root = Path(sys.argv[1])
(root / "picture").mkdir(parents=True, exist_ok=True)
def bmp565(path, width=135, height=240, padded=True, top_down=False):
    stride = (width*2+3)&~3 if padded else width*2
    header = bytearray(70)
    header[:2] = b"BM"
    for off, value in [(2,70+stride*height),(10,70),(14,56),(18,width),
                       (22,-height if top_down else height),(30,3),
                       (34,stride*height),(54,0xf800),(58,0x7e0),(62,0x1f)]:
        struct.pack_into("<I",header,off,value & 0xffffffff)
    struct.pack_into("<HH",header,26,1,16)
    path.write_bytes(header+(struct.pack("<H",0xf800)*width+bytes(stride-width*2))*height)
for name,padded,top_down in [("legacy.bmp",False,False),("top-down.bmp",True,True),("padded.bmp",True,False)]:
    bmp565(root/name,padded=padded,top_down=top_down)
red=Image.new("RGB",(160,100),(255,0,0))
red.save(root/"rgb24.bmp");red.save(root/"sample.jpg",quality=95)
(root/"broken.bmp").write_bytes(b"BM"+bytes(100))
for i in range(11):
    im=Image.new("RGB",(240,135),[(88,127,139),(193,161,117),(119,142,121)][i%3])
    d=ImageDraw.Draw(im)
    d.ellipse((145-i*4,18,169-i*4,42),fill=(237,213,156))
    d.polygon([(0,105),(65,36+i*2),(124,99),(169,48),(240,112),(240,135),(0,135)],fill=(46+i*3,71,75))
    d.polygon([(0,125),(94,83),(174,119),(240,96),(240,135),(0,135)],fill=(25,43+i*2,47))
    im.save(root/"picture"/f"photo{i}.jpg",quality=90)
