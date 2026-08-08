#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the ftk-fx project.
"""Capture every screenshot in a manifest.

One process per shot, so nothing a shot leaves behind can reach the next one.
Each shot writes a PNG and a JSON sidecar holding the bounding box and the
visible text of every tagged widget.

    etc/Screenshots/build_screenshots.py etc/Screenshots/screenshots.json \\
        --ftk-fx ../build-Debug/bin/ftk-fx/ftk-fx --out /tmp/shots

On headless Linux, run under a virtual display:
    xvfb-run -a etc/Screenshots/build_screenshots.py ...
"""

import argparse
import json
import pathlib
import subprocess
import sys
import tempfile

HERE = pathlib.Path(__file__).resolve().parent  # etc/Screenshots
REPO_ROOT = HERE.parent.parent                  # repository root


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("manifest", type=pathlib.Path)
    ap.add_argument("--ftk-fx", required=True, help="path to the ftk-fx executable")
    ap.add_argument("--root", type=pathlib.Path, default=REPO_ROOT,
                    help="working directory for ftk-fx (default: repo root)")
    ap.add_argument("--out", type=pathlib.Path, default=None,
                    help="where to write the PNGs and sidecars (default: temp)")
    ap.add_argument("--only", nargs="*", help="capture only these shot ids")
    args = ap.parse_args()

    # Resolve to absolutes so they survive the subprocess cwd change.
    manifest = args.manifest.resolve()
    root = args.root.resolve()
    ftk_fx = str(pathlib.Path(getattr(args, "ftk_fx")).resolve())
    out = (args.out.resolve() if args.out
           else pathlib.Path(tempfile.mkdtemp(prefix="ftk-fx-shots-")))
    out.mkdir(parents=True, exist_ok=True)

    data = json.loads(manifest.read_text())
    ids = [s["id"] for s in data["shots"]]
    if args.only:
        ids = [i for i in ids if i in args.only]

    # Redirect the settings to a throwaway file so a local run never reads or
    # overwrites the real ones. -resetSettings still guarantees each shot starts
    # from the defaults whatever a previous shot wrote there.
    settings = out / "capture-settings.json"

    failures = []
    for shot_id in ids:
        cmd = [
            ftk_fx,
            "-resetSettings",
            "-settingsFile", str(settings),
            "-captureManifest", str(manifest),
            "-captureShot", shot_id,
            "-captureOutput", str(out),
        ]
        print(f"[capture] {shot_id} (cwd={root})")
        if subprocess.run(cmd, cwd=root).returncode != 0:
            failures.append(shot_id)
            print(f"  ! capture failed for {shot_id}", file=sys.stderr)
            continue
        if not (out / f"{shot_id}.json").exists():
            failures.append(shot_id)
            print(f"  ! no sidecar produced for {shot_id}", file=sys.stderr)

    if failures:
        sys.exit(f"\n{len(failures)} shot(s) failed: {', '.join(failures)}")
    print(f"\nDone: {len(ids)} shot(s) -> {out}")


if __name__ == "__main__":
    main()
