#!/usr/bin/env python3
"""Package 2.2.2 after the native build; retain the tested installer protocol."""
from pathlib import Path
import base64
import hashlib
import json
import re
import shutil
import subprocess
import sys
import zipfile

ROOT=Path(__file__).resolve().parents[1]

def verify_sources():
    baseline=json.loads((ROOT/"releases/v2.2.1/manifest.json").read_text(encoding="utf-8"))
    exceptions={"firmware/src/ui_graph.c","firmware/src/felucca.c","tools/build.py"}
    for name,digest in baseline["source_sha256_lf"].items():
        if name not in exceptions:
            assert hashlib.sha256((ROOT/name).read_bytes().replace(b"\r\n",b"\n")).hexdigest()==digest,name
    for name in exceptions:
        old=subprocess.check_output(["git","show",f"v2.2.1:{name}"],cwd=ROOT).replace(b"\r\n",b"\n")
        now=(ROOT/name).read_bytes().replace(b"\r\n",b"\n")
        if name.endswith("felucca.c"):
            assert now==old.replace(b'Sillage 2.2.1 FX test',b'Sillage 2.2.2'),name
        elif name.endswith("build.py"):
            assert now==old.replace(b'FM-1_946221',b'FM-1_946222'),name
        else:
            # Only HOME's compact names renderer changes; all other rendering stays intact.
            start=old.index(b"/* (2.5, #97, the user's 8C) MENU > LARGE on HOME:")
            end=old.index(b"\n/* (Skiff U5)",start)
            new_start=now.index(b"/* MENU > LARGE on HOME:")
            new_end=now.index(b"\n/* (Skiff U5)",new_start)
            assert now[:new_start]==old[:start] and now[new_end:]==old[end:],name
    print("Validated 2.2.1 audio and other compiled inputs unchanged; only HOME renderer and version metadata changed.")

def package():
    verify_sources()
    fw=(ROOT/"build/felucca.fwsc").read_bytes()
    ident="".join(chr((fw[i*0x30+0x2f]-i-1)&255) for i in range(20) if fw[i*0x30+0x2f]!=0x7d)
    assert ident=="FM-1_946222",ident
    assert b"FELUCCA-LOADER-1" in fw
    digest=hashlib.sha256(fw).hexdigest()
    html=(ROOT/"docs/installer.html").read_text(encoding="utf-8")
    for name,value in [("payload",base64.b64encode(fw).decode()),("expected",digest),("product",ident)]:
        html,n=re.subn(r'const '+name+r'="[^"]*"',lambda m:f'const {name}="{value}"',html)
        assert n==1,name
    html=html.replace("Sillage 2.2.1","Sillage 2.2.2")
    (ROOT/"docs/installer.html").write_text(html,encoding="utf-8")
    (ROOT/"downloads/Sillage.fwsc").write_bytes(fw)
    meta_dir=ROOT/"releases/v2.2.2"
    meta_dir.mkdir(parents=True,exist_ok=True)
    manifest={"version":"2.2.2","identity":ident,"firmware_sha256":digest,"base":"v2.2.1","changes":"HOME Large: two columns of four; audio source inputs unchanged.","source_sha256_lf":{name:hashlib.sha256((ROOT/name).read_bytes().replace(b"\r\n",b"\n")).hexdigest() for name in json.loads((ROOT/"releases/v2.2.1/manifest.json").read_text())["source_sha256_lf"]}}
    (meta_dir/"manifest.json").write_text(json.dumps(manifest,indent=2)+"\n",encoding="utf-8")
    out=ROOT/"dist/release-2.2.2"
    out.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(ROOT/"downloads/Sillage.fwsc",out/"Sillage.fwsc")
    source=ROOT/"dist/Sillage-sources-2.2.2.zip"
    names=subprocess.check_output(["git","ls-files","-c","-o","--exclude-standard","-z"],cwd=ROOT).decode().split("\0")
    with zipfile.ZipFile(source,"w",zipfile.ZIP_DEFLATED) as z:
        for name in sorted(set(names)):
            p=ROOT/name
            if name and p.is_file() and not name.startswith((".git/","dist/","build/","downloads/")):
                z.write(p,name)
    share=out/"Sillage-v2.2.2.zip"
    with zipfile.ZipFile(share,"w",zipfile.ZIP_DEFLATED) as z:
        z.writestr("Sillage.fwsc",fw)
        z.write(ROOT/"docs/installer.html","Installer Sillage.html")
        z.write(ROOT/"docs/editor.html","Editeur Sillage.html")
        z.write(source,"Sillage-sources.zip")
        for name in ["Prise-en-main.md","Manuel-de-reference.md","RELEASE_NOTES_2.2.2.md","LICENSE","LICENSING.md"]:
            z.write(ROOT/name,name)
        z.writestr("READ ME.txt","Sillage 2.2.2 for M-VAVE FM-1. Extract the ZIP, back up your projects, then open Installer Sillage.html in desktop Chrome or Edge. HOME Large now shows all eight tracks. Audio processing is unchanged from 2.2.1. Based on Skiff and Felucca.\n")
    with zipfile.ZipFile(share) as z:
        assert z.testzip() is None and z.read("Sillage.fwsc")==fw
    assert base64.b64decode(re.search(r'const payload="([^"]+)',html)[1])==fw
    (out/"SHA256SUMS.txt").write_text("".join(f"{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}\n" for p in [out/"Sillage.fwsc",share]),encoding="ascii")
    print(f"Packaged Sillage 2.2.2: {ident}, SHA256 {digest}")

if __name__=="__main__":
    verify_sources() if "--verify-only" in sys.argv else package()
