#!/usr/bin/python3

import sys
import os
import uuid
import json
from shutil import copy, copytree

if len(sys.argv) < 3:
    print("Usage: packmap.py sysroot maps ...");
    exit(0)

sysroot = sys.argv[1]
assetpath = os.path.join("map-assets")
assetfullpath = os.path.join(sysroot, assetpath)

assetdict = {}
assetdirs = {}
pathdict = {}

def mkdirp(dirname):
    try:
        os.mkdir(dirname)
    except FileExistsError:
        pass

def fprint(x):
    sys.stderr.write(x)
    sys.stderr.write("\n")

mkdirp(sysroot)
mkdirp(assetpath)

def getParent(path):
    d = os.path.dirname(path)

    if d in assetdirs:
        return assetdirs[d]
    else:
        duuid = str(uuid.uuid4())
        #did = os.path.join("/", assetpath, duuid)
        did = os.path.join(assetpath, duuid)
        cpath = os.path.join(assetfullpath, duuid)
        assetdirs.update({d: did})
        fprint("dir:  %s => %s" % (d, did))
        copytree(d, cpath, copy_function=copy)
        return did

def getAsset(source):
    base = os.path.basename(source)
    out = os.path.join(getParent(source), base)

    fprint("file: %s => %s" % (base, out))
    return out

def replacePaths(mapjson):
    for key in mapjson:
        if isinstance(mapjson[key], dict):
            replacePaths(mapjson[key])

    if "sourceFile" in mapjson:
        mapjson["sourceFile"] = getAsset(mapjson["sourceFile"])

def saveMap(name, mapjson):
    try:
        f = open(name, "w")
        f.write(json.dumps(mapjson))

    except Exception as e:
        fprint("couldn't save output map %s: %s", (name, str(e)))

for mapfile in sys.argv[2:]:
    if mapfile in pathdict:
        continue

    base = os.path.basename(mapfile)
    outname = os.path.join(sysroot, base)
    pathdict.update({mapfile: outname})

    mapjson = json.load(open(mapfile))
    fprint("have map file: %s => %s" % (mapfile, pathdict[mapfile]))
    replacePaths(mapjson)
    saveMap(outname, mapjson)
