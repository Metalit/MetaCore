"""Run the C++ calculator and parser against both API snapshot datasets."""
import argparse
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parent.parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    compiler = parser.add_mutually_exclusive_group()
    compiler.add_argument("--cxx", default=os.environ.get("CXX", "c++"), help="C++20 compiler (GCC or Clang)")
    compiler.add_argument("--zig", help="Use zig c++ (also supports Windows)")
    args = parser.parse_args()
    build = ROOT / "build" / "tests"
    build.mkdir(parents=True, exist_ok=True)
    output = build / ("vectors.exe" if os.name == "nt" else "vectors")
    command = [args.zig, "c++"] if args.zig else [args.cxx]
    command += ["-std=c++20", "-O2", "-DRAPIDJSON_MACROS_GCC_TEST", "-I", str(ROOT / "tests/stubs"), "-I", str(ROOT / "tests"), "-I", str(ROOT / "shared"), "-I", str(ROOT / "include"), "-I", str(ROOT / "extern/includes"), "-I",
                str(ROOT / "extern/includes/beatsaber-hook/shared/rapidjson/include")]
    if args.zig:
        command.append("-Wno-nullability-completeness")
    command += [str(ROOT / name) for name in ("src/ppv4.cpp", "src/ppv4parser.cpp", "src/ppv4map.cpp", "tests/ppv4.cpp")]
    subprocess.run(command + ["-o", str(output)], check=True)
    for fixture in ("scores-2026-10-30.json", "scores-2026-11-01.json"):
        subprocess.run([str(output), str(ROOT / "tests" / fixture)], check=True)


if __name__ == "__main__":
    main()
