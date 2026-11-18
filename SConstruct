#!/usr/bin/env python

import glob
import os

from config import can_build, get_opts
from gdlive2d_methods import link_cubism_core_library

CUBISM_FRAMEWORK_SRC_PATHS = (
    ".",
    "Effect",
    "Id",
    "Math",
    "Model",
    # "Motion",
    "Physics",
    "Rendering",
    "Type",
    "Utils",
)

CUBISM_FRAMEWORK_EXCLUDE_FILES = (
    "CubismCdiJson.cpp",
    "CubismViewMatrix.cpp",
    "CubismUserModel.cpp",
    # "CubismRenderer.cpp",
)

env = SConscript("lib/godot-cpp/SConstruct").Clone()

opts = Variables([], ARGUMENTS)
for opt in get_opts(env["platform"]):
    opts.Add(opt)
opts.Update(env)

if not can_build(env, env["platform"]):
    exit(255)

thirdparty_cubism_core_dir = os.path.join(env["cubism_sdk"], "Core")
thirdparty_cubism_framework_src_dir = os.path.join(env["cubism_sdk"], "Framework", "src")

env.Prepend(CPPPATH=[os.path.join(thirdparty_cubism_core_dir, "include")])
env.Prepend(CPPPATH=[thirdparty_cubism_framework_src_dir])

# Thirdparty source files

sources = []

thirdparty_cubism_framework_sources = [
    file
    for path in CUBISM_FRAMEWORK_SRC_PATHS
    for file in glob.glob(os.path.join(thirdparty_cubism_framework_src_dir, path, "*.cpp"))
    if os.path.basename(file) not in CUBISM_FRAMEWORK_EXCLUDE_FILES
]

sources += thirdparty_cubism_framework_sources

link_cubism_core_library(env, thirdparty_cubism_core_dir)

# Godot source files

sources += glob.glob("*.cpp")
sources += glob.glob("src/*.cpp")
sources += glob.glob("src/editor/*.cpp")

BIN_DIR = env.Dir("#bin").abspath
OBJ_DIR = os.path.join(BIN_DIR, "obj")

BIN_NAME = "libgdlive2d{}{}".format(env["suffix"], env["SHLIBSUFFIX"])


def _resolve_object_target(env, src, obj_dir):
    root_abs = env.Dir("#").abspath
    src_abs = env.File(src).abspath
    src_rel = os.path.normpath(os.path.relpath(src_abs, root_abs))
    src_rel_stem = os.path.splitext(src_rel)[0]
    return os.path.join(root_abs, obj_dir, src_rel_stem)


library = env.SharedLibrary(
    os.path.join(BIN_DIR, BIN_NAME),
    source=[env.SharedObject(_resolve_object_target(env, s, OBJ_DIR), s) for s in sources],
)

installed_library = env.InstallAs(
    os.path.join("addons/gdlive2d/bin", BIN_NAME),
    next(n for n in library if os.path.basename(n.get_abspath()) == BIN_NAME),
)

env.NoCache(library)
Default(installed_library)
