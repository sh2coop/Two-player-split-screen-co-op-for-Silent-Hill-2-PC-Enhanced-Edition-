#!/usr/bin/env python3
"""make_update_release.py - the files to attach to a GitHub Release, so the in-game updater can install the build.

usage: make_update_release.py BUILD_DIR OUT_DIR SIGNING_KEY.pem [--with-zip]

BUILD_DIR holds what build.sh made (out/stage): sh2coop.asi, sh2coop_check.exe, "Install co-op.exe", README.txt
(the player README), VERSION.txt, and "SH2 co-op mod <build>.zip" there or one folder up (out/). OUT_DIR gets the release assets, every name without spaces
(GitHub turns spaces in asset names into dots):
  sh2coop.asi  sh2coop_check.exe  sh2coop-README.txt  sh2coop-VERSION.txt   (what the updater installs)
  sh2coop-manifest.txt  sh2coop-manifest.sig                                (what the updater checks)
  SH2-co-op-mod-<build>.zip  Install-co-op.exe     (only with --with-zip: for people installing the first time)
The release's TAG must be the build name (e.g. P33w); the manifest says so.
The signing key is the release key's private half (PEM). Keep it off GitHub: anyone holding it can ship code to
every player who presses Update.
"""
import hashlib, os, re, shutil, sys
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric.utils import decode_dss_signature, encode_dss_signature

src, out, keyfile = sys.argv[1:4]
asi = open(os.path.join(src, "sh2coop.asi"), "rb").read()
m = re.search(rb"sh2coop PC build ([A-Za-z0-9]+)", asi)
if not m:
    sys.exit("no build name in sh2coop.asi")
build = m.group(1).decode()
if b"test: James 1 teleported" in asi:
    sys.exit("that is a test-tools build, not a release build")
os.makedirs(out, exist_ok=True)

# asset name, local name (what the updater writes in the game folder), source file
updated = [
    ("sh2coop.asi", "sh2coop.asi", "sh2coop.asi"),
    ("sh2coop_check.exe", "sh2coop_check.exe", "sh2coop_check.exe"),
    ("sh2coop-README.txt", "sh2coop README.txt", "README.txt"),
    ("sh2coop-VERSION.txt", "sh2coop VERSION.txt", "VERSION.txt"),
]
lines = ["sh2coop-update 1", f"build {build}", f"tag {build}"]
for asset, local, source in updated:
    data = open(os.path.join(src, source), "rb").read()
    open(os.path.join(out, asset), "wb").write(data)
    lines.append("\t".join(["file", asset, local, str(len(data)), hashlib.sha256(data).hexdigest()]))
manifest = ("\n".join(lines) + "\n").encode()
open(os.path.join(out, "sh2coop-manifest.txt"), "wb").write(manifest)

key = serialization.load_pem_private_key(open(keyfile, "rb").read(), password=None)
r, s = decode_dss_signature(key.sign(manifest, ec.ECDSA(hashes.SHA256())))
sig = (r.to_bytes(32, "big") + s.to_bytes(32, "big")).hex()
key.public_key().verify(encode_dss_signature(r, s), manifest, ec.ECDSA(hashes.SHA256()))   # the written signature checks out
open(os.path.join(out, "sh2coop-manifest.sig"), "w").write(sig + "\n")

# optional extras for first installs (the updater does not use them)
if "--with-zip" in sys.argv:
    zipname = f"SH2 co-op mod {build}.zip"
    zippath = next(p for p in (os.path.join(src, zipname), os.path.join(src, "..", zipname)) if os.path.exists(p))
    shutil.copyfile(zippath, os.path.join(out, f"SH2-co-op-mod-{build}.zip"))
    shutil.copyfile(os.path.join(src, "Install co-op.exe"), os.path.join(out, "Install-co-op.exe"))
print(f"release {build}: tag {build}, assets in {out}:")
for n in sorted(os.listdir(out)):
    print(f"  {n:28s} {os.path.getsize(os.path.join(out, n)):>9}")
