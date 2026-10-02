#!/usr/bin/env python3
"""Installs Qt for one compiler from Qt's online repository, the way aqtinstall would.

aqtinstall 3.3.0 (the newest release) can't install Qt 6.12 on Windows: from 6.12, Qt
keeps each compiler's packages in its own folder (qt6_6120/qt6_6120_msvc2022_64/),
while aqtinstall still looks for one combined folder. This script reads that folder's
package index (Updates.xml, checked against its SHA-256), downloads the archives of the
base package and the requested add-ons (each checked against its SHA-1), and unpacks
them with 7-Zip into <out>/<version>/<compiler>/, the same layout aqtinstall produces
(in this layout the archives hold paths like bin/Qt6Core.dll, with no version prefix).

Example (as the Windows workflow runs it):
  python install_qt.py --version 6.12.0 --arch win64_msvc2022_64 --out C:/Qt --addons qttasktree
"""

import argparse
import hashlib
import pathlib
import subprocess
import sys
import tempfile
import urllib.request
import xml.etree.ElementTree as ET

BASE = "https://download.qt.io/online/qtsdkrepository"


def fetch(url: str) -> bytes:
    with urllib.request.urlopen(url, timeout=120) as response:
        return response.read()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--host", default="windows_x86", help="repository host folder (default: windows_x86)")
    parser.add_argument("--version", required=True, help="e.g. 6.12.0")
    parser.add_argument("--arch", required=True, help="e.g. win64_msvc2022_64")
    parser.add_argument("--out", required=True, help="where Qt goes (<out>/<version>/<compiler>/)")
    parser.add_argument("--addons", nargs="*", default=[], help="add-on modules, e.g. qttasktree")
    parser.add_argument("--dry-run", action="store_true", help="list the archives, download nothing")
    args = parser.parse_args()

    v = args.version.replace(".", "")
    compiler = args.arch.removeprefix("win64_")
    folder = f"{BASE}/{args.host}/desktop/qt6_{v}/qt6_{v}_{compiler}"

    index = fetch(f"{folder}/Updates.xml")
    expected = fetch(f"{folder}/Updates.xml.sha256").decode().split()[0]
    if hashlib.sha256(index).hexdigest() != expected:
        sys.exit("Updates.xml doesn't match its SHA-256")

    wanted = [f"qt.qt6.{v}.{args.arch}"] + [f"qt.qt6.{v}.addons.{m}.{args.arch}" for m in args.addons]
    packages = {p.findtext("Name"): p for p in ET.fromstring(index).findall("PackageUpdate")}
    missing = [name for name in wanted if name not in packages]
    if missing:
        sys.exit("not in the repository: " + ", ".join(missing))

    out = pathlib.Path(args.out)
    target = out / args.version / compiler
    target.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as tmp:
        for name in wanted:
            package = packages[name]
            version = package.findtext("Version")
            archives = [a.strip() for a in (package.findtext("DownloadableArchives") or "").split(",") if a.strip()]
            for archive in archives:
                url = f"{folder}/{name}/{version}{archive}"
                print(("would get " if args.dry_run else "getting ") + url, flush=True)
                if args.dry_run:
                    continue
                data = fetch(url)
                sha1 = fetch(url + ".sha1").decode().split()[0]
                if hashlib.sha1(data).hexdigest() != sha1:
                    sys.exit(f"{archive} doesn't match its SHA-1")
                path = pathlib.Path(tmp) / archive
                path.write_bytes(data)
                subprocess.run(["7z", "x", "-y", "-bso0", "-bsp0", f"-o{target}", str(path)], check=True)
                path.unlink()

    print(f"Qt {args.version} ({compiler}) is in {target}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
