import glob
import os


def choose_path(paths, prompt="Please select a path"):
    if not paths:
        print("No paths available.")
        return None

    print(prompt)
    for i, p in enumerate(paths, start=1):
        print(f"{i}. {p}")
    print("0. Cancel")

    while True:
        try:
            raw = input(f"Enter number (default={len(paths)}): ")
            if raw == "":
                raw = str(len(paths))
            idx = int(raw)
            if idx == 0:
                return None
            if 1 <= idx <= len(paths):
                return paths[idx - 1]
            else:
                print(f"Please enter a number between 0 and {len(paths)}.")
        except ValueError:
            print("Invalid input, please enter a number.")


def can_build(env, platform):
    if not env.get("cubism_sdk"):
        sdk_glob = os.path.join(os.path.dirname(os.path.abspath(__file__)), "lib", "CubismSdkForNative*")
        matches = glob.glob(sdk_glob)
        selected = ""
        if len(matches) == 1:
            selected = matches[0]
            print(f"Automatically selected the only Cubism SDK path: {selected}")
        elif len(matches) > 1:
            selected = choose_path(matches, "Multiple Cubism SDK paths found, please select:")
            if selected is None:
                print("User cancelled.")
            else:
                print(f"Selected: {selected}")
        else:
            print("No Cubism SDK path found.")

        if selected:
            env["cubism_sdk"] = selected

    library = "module"
    if "GDEXTENSION" in env["CPPDEFINES"]:
        library = "plugin"

    if not env.get("cubism_sdk"):
        print(f"Failed to build the live2d {library} without 'cubism_sdk' option.")
        return False

    if platform in ("windows"):
        if platform == "windows" and env["arch"] not in ("x86_32", "x86_64"):
            print(f"Failed to build the live2d {library}, only x86_64 and x86_32 are supported on Windows.")
            return False
        return True
    else:
        print(f"The live2d {library} is not supported for platform {platform} right now.")
        return False


def get_opts(platform):
    return [
        ("cubism_sdk", "Path to the Cubism SDK"),
    ]


def configure(env):
    pass


def get_doc_classes():
    return []


def get_doc_path():
    return "doc_classes"
