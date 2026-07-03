#!/usr/bin/env python3
"""Provision the Epic Online Services (EOS) SDK into the EOSIntegrationKit plugin.

The EIK plugin ships its Build.cs/XML but NOT the EOS SDK binaries/headers
(large + licensed, so they're gitignored). The EOS tasks (66-68) cannot compile
until the SDK is placed at:
    <task>/{start,solution}/Plugins/EOSIntegrationKit/Source/ThirdParty/EIKSDK/
        Bin/EOSSDK-Win64-Shipping.dll
        Lib/EOSSDK-Win64-Shipping.lib
        Include/...            (the ENTIRE tree, incl. the Windows/ platform subdir)

Download the EOS SDK from the Epic Dev Portal, then run:
    python setup_eos_sdk.py --sdk "C:/path/to/EOS-SDK-.../SDK"
(omit --sdk to auto-detect the newest EOS-SDK-* under ~/Downloads).
"""
import argparse, glob, os, shutil, sys

def find_sdk():
    home = os.path.expanduser("~")
    cands = sorted(glob.glob(os.path.join(home, "Downloads", "EOS-SDK-*", "SDK")), reverse=True)
    return cands[0] if cands else None

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--sdk", default=None, help="Path to the EOS SDK dir (the folder containing Bin/Lib/Include)")
    ap.add_argument("--tasks-dir", default="tasks_unreal")
    args = ap.parse_args()

    sdk = args.sdk or find_sdk()
    if not sdk or not os.path.isdir(sdk):
        sys.exit("EOS SDK not found. Pass --sdk <path-to-SDK-dir>.")
    dll = os.path.join(sdk, "Bin", "EOSSDK-Win64-Shipping.dll")
    lib = os.path.join(sdk, "Lib", "EOSSDK-Win64-Shipping.lib")
    inc = os.path.join(sdk, "Include")
    for p in (dll, lib, inc):
        if not os.path.exists(p):
            sys.exit(f"EOS SDK incomplete — missing {p}")

    pattern = os.path.join(args.tasks_dir, "ue_task_*", "*", "Plugins",
                           "EOSIntegrationKit", "Source", "ThirdParty", "EIKSDK")
    eik_dirs = [d for d in glob.glob(pattern)
                if (os.sep + "start" + os.sep) in d or (os.sep + "solution" + os.sep) in d]
    if not eik_dirs:
        sys.exit(f"No EIKSDK dirs found under {args.tasks_dir} — nothing to provision.")

    for d in sorted(eik_dirs):
        os.makedirs(os.path.join(d, "Bin"), exist_ok=True)
        os.makedirs(os.path.join(d, "Lib"), exist_ok=True)
        os.makedirs(os.path.join(d, "Include"), exist_ok=True)
        shutil.copy2(dll, os.path.join(d, "Bin", os.path.basename(dll)))
        shutil.copy2(lib, os.path.join(d, "Lib", os.path.basename(lib)))
        # ENTIRE Include tree (incl. Windows/ — the bit that's easy to miss)
        shutil.copytree(inc, os.path.join(d, "Include"), dirs_exist_ok=True)
        ok = os.path.isfile(os.path.join(d, "Include", "Windows", "eos_Windows.h"))
        print(f"  provisioned {d}  (Windows/eos_Windows.h {'OK' if ok else 'MISSING'})")
    print(f"\nDone: {len(eik_dirs)} EIKSDK dir(s) provisioned from {sdk}")

if __name__ == "__main__":
    main()
