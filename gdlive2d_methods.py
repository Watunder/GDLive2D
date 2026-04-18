import os


def get_library(core_dir, *paths):
    return os.path.join(core_dir, "lib", *paths)


def link_cubism_core_library(env, core_dir):
    if env["platform"] == "windows":
        arch = env["arch"]
        if arch == "x86_32":
            arch = "x86"
        rt = "MDd" if env["debug_crt"] else "MT" if env["use_static_cpp"] else "MD"
        lib = get_library(
            core_dir,
            "windows",
            arch,
            env["MSVC_VERSION"].replace(".", ""),
            "Live2DCubismCore_{}.lib".format(rt),
        )
        env.Append(LINKFLAGS=[lib])
    else:
        raise ValueError("The Cubism Core library linking is not implemented for platform: " + env["platform"])
