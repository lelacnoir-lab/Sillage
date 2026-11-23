#!/usr/bin/env python3
"""Package the exact physically validated binary; never silently rebuild it."""
from pathlib import Path
import base64, hashlib, json, re, shutil, subprocess, zipfile
ROOT = Path(__file__).resolve().parents[1]
META = ROOT / "releases/v2.2.1/manifest.json"
meta = json.loads(META.read_text(encoding="utf-8"))
fw = (ROOT / meta["firmware"]).read_bytes()
assert hashlib.sha256(fw).hexdigest() == meta["firmware_sha256"], "Firmware differs from the validated file"
identity = "".join(chr((fw[i*0x30+0x2f]-i-1)&255) for i in range(20) if fw[i*0x30+0x2f] != 0x7d)
assert identity == meta["identity"], identity
for name, digest in meta["source_sha256_lf"].items():
    assert hashlib.sha256((ROOT/name).read_bytes().replace(b"\r\n", b"\n")).hexdigest() == digest, name
installer = (ROOT/"docs/installer.html").read_text(encoding="utf-8")
assert base64.b64decode(re.search(r'const payload="([^"]+)', installer)[1]) == fw
assert re.search(r'const expected="([^"]+)', installer)[1] == meta["firmware_sha256"]
assert re.search(r'const product="([^"]+)', installer)[1] == meta["identity"]
out = ROOT/"dist/release"
out.mkdir(parents=True, exist_ok=True)
shutil.copyfile(ROOT/meta["firmware"], out/"Sillage.fwsc")
source = ROOT/"dist/Sillage-sources-2.2.1.zip"
names = subprocess.check_output(["git", "ls-files", "-c", "-o", "--exclude-standard", "-z"], cwd=ROOT).decode().split("\0")
with zipfile.ZipFile(source, "w", zipfile.ZIP_DEFLATED) as z:
    for name in sorted(set(names)):
        p = ROOT/name
        if name and p.is_file() and not name.startswith((".git/", "dist/", "build/", "downloads/")):
            z.write(p, name)
share = out/"Sillage-v2.2.1.zip"
with zipfile.ZipFile(share, "w", zipfile.ZIP_DEFLATED) as z:
    z.writestr("Sillage.fwsc", fw)
    z.write(ROOT/"docs/installer.html", "Installer Sillage.html")
    z.write(ROOT/"docs/editor.html", "Editeur Sillage.html")
    z.write(ROOT/"docs/editor-guide.txt", "Guide editeur.txt")
    for name in ["Prise-en-main.md", "Manuel-de-reference.md", "RELEASE_NOTES_2.2.1.md", "LICENSE", "LICENSING.md"]:
        z.write(ROOT/name, name)
    z.write(source, "Sillage-sources.zip")
    z.writestr("LIRE MOI - READ ME.txt", "SILLAGE 2.2.1 — M-VAVE FM-1\n\nExtract the ZIP, then open Installer Sillage.html in desktop Chrome or Edge. Back up your projects first and follow the USB-MIDI installer. Editeur Sillage.html is the offline editor; the public editor is https://lelacnoir-lab.github.io/Sillage/editor.html .\n\nDécompresse le ZIP puis ouvre Installer Sillage.html dans Chrome ou Edge sur ordinateur. Sauvegarde tes projets et suis les étapes de l'installateur USB MIDI. Editeur Sillage.html est l'éditeur hors ligne.\n\nBased on Skiff and Felucca. This package contains the exact physically validated 2.2-based firmware with FX HOLD and a dry-only filter; original reverb processing retained. USB identity FM-1_946221; internal version label Sillage 2.2.1 FX test.\n")
with zipfile.ZipFile(share) as z:
    assert z.testzip() is None
    assert z.read("Sillage.fwsc") == fw
    assert z.read("Editeur Sillage.html") == (ROOT/"docs/editor.html").read_bytes()
(out/"SHA256SUMS.txt").write_text("".join(f"{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}\n" for p in [out/"Sillage.fwsc", share]), encoding="ascii")
print(f"Verified {len(meta['source_sha256_lf'])} source files; identity {identity}; firmware SHA256 {meta['firmware_sha256']}")
print(f"Release ZIP: {share.name}, {share.stat().st_size} bytes")
