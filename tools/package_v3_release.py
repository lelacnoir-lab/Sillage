#!/usr/bin/env python3
"""Verify and package the unchanged Sillage V3 candidate; no upload or rebuild."""
from pathlib import Path
import argparse, base64, hashlib, json, re, zipfile

ROOT = Path(__file__).resolve().parents[1]
def digest(data): return hashlib.sha256(data).hexdigest()
def identity(data):
    return ''.join(chr((data[i*0x30+0x2f]-i-1)&255) for i in range(20) if data[i*0x30+0x2f] != 0x7d)
def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--check-build',type=Path)
    args=ap.parse_args()
    meta=json.loads((ROOT/'releases/v3.0.0/manifest.json').read_text(encoding='utf8'))
    fw=(ROOT/meta['firmware']).read_bytes()
    assert digest(fw)==meta['firmware_sha256'], 'Candidate firmware hash differs'
    assert identity(fw)==meta['identity'], 'Candidate identity differs'
    for name,expected in meta['source_sha256'].items():
        assert digest((ROOT/name).read_bytes().replace(b'\r\n',b'\n'))==expected, 'Pinned source/asset changed: '+name
    installer=(ROOT/'docs/installer.html').read_text(encoding='utf8')
    assert base64.b64decode(re.search(r'const payload="([^"]+)"',installer)[1])==fw
    assert re.search(r'const expected="([^"]+)"',installer)[1]==meta['firmware_sha256']
    assert re.search(r'const product="([^"]+)"',installer)[1]==meta['identity']
    if args.check_build:
        data=args.check_build.read_bytes()
        assert identity(data)==meta['identity'], 'Rebuild identity differs'
        assert b'FELUCCA-LOADER-1' in data, 'Rebuild loader missing'
        print('Rebuild structural check passed; byte-identical to candidate:',data==fw)
        print('Rebuild SHA256:',digest(data))
        return
    out=ROOT/'dist/release-v3';out.mkdir(parents=True,exist_ok=True)
    (out/'Sillage.fwsc').write_bytes(fw)
    # Explicit source roots: no SDK, private data, git metadata or build output.
    source=out/'Sillage-V3-sources.zip'
    names=[]
    for folder in ['firmware','tools','tests','generated','LICENSES','assets','design','web','.github','docs']:
        for p in (ROOT/folder).rglob('*'):
            if not p.is_file() or any(x in p.parts for x in ['node_modules','__pycache__','.git','out']): continue
            if p.suffix in ['.pyc','.fwsc','.elf','.bin']: continue
            names.append(p)
    for name in ['LICENSE','LICENSING.md','README.md','BUILDING.md','CONTRIBUTING.md','DESIGN.md','SMK37.md','recompiler-fm1.sh','build.sh','wsl-build.sh','Manuel-de-reference.md','Prise-en-main.md','RELEASE_NOTES_V3.md','MISE-EN-LIGNE-GITHUB.md','fmhub.json']:
        names.append(ROOT/name)
    with zipfile.ZipFile(source,'w',zipfile.ZIP_DEFLATED) as z:
        for p in sorted(set(names)):z.write(p,p.relative_to(ROOT).as_posix())
        z.write(ROOT/'releases/v3.0.0/manifest.json','releases/v3.0.0/manifest.json')
        z.writestr('downloads/Sillage.fwsc',fw)
    share=out/'Sillage-V3.zip'
    with zipfile.ZipFile(share,'w',zipfile.ZIP_DEFLATED) as z:
        z.writestr('Sillage.fwsc',fw)
        for src,dest in [('docs/installer.html','Installer Sillage V3.html'),('docs/editor.html','Editeur Sillage.html'),('docs/editor-guide.txt','Guide editeur.txt'),('docs/Sillage-V3-Quick-Start-EN.pdf','Sillage-V3-Quick-Start-EN.pdf'),('Manuel-de-reference.md','Manuel-de-reference.md'),('Prise-en-main.md','Prise-en-main.md'),('RELEASE_NOTES_V3.md','RELEASE_NOTES_V3.md'),('LICENSE','LICENSE'),('LICENSING.md','LICENSING.md')]:z.write(ROOT/src,dest)
        z.write(source,'Sillage-V3-sources.zip')
        z.write(ROOT/'RELEASE_NOTES_V3.md','release-notes-v3.md')
        z.writestr('LIRE MOI - READ ME.txt','SILLAGE V3 — M-VAVE FM-1\n\nExtract the ZIP. Back up projects, sounds and samples. Open Installer Sillage V3.html in desktop Chrome/Edge, connect one FM-1 by USB and follow the prompts. Internal version: Sillage V3 beta 1; identity FM-1_946301. This is the unchanged compiled candidate. The web editor is https://lelacnoir-lab.github.io/Sillage/editor.html . Read the enclosed PDF and V3 release notes. Previous versions: https://github.com/lelacnoir-lab/Sillage/releases .\n\nDécompresser le ZIP, sauvegarder ses données, puis ouvrir Installer Sillage V3.html dans Chrome/Edge. Consulter le manuel et les notes V3. EVOLVE et transposition sont temporaires et non sauvegardés.\n')
    for file in [source,share]:
        with zipfile.ZipFile(file) as z:assert z.testzip() is None
    with zipfile.ZipFile(share) as z:assert z.read('Sillage.fwsc')==fw
    files=[out/'Sillage.fwsc',share,source]
    (out/'SHA256SUMS.txt').write_text(''.join(f'{digest(f.read_bytes())}  {f.name}\n' for f in files),encoding='ascii')
    print(f'Verified {len(meta["source_sha256"])} pinned files; exact installer and firmware match.')
    print('Prepared locally:',out)
if __name__=='__main__':main()
