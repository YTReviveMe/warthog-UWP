#!/usr/bin/env python3
import argparse
import shutil
import subprocess
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="mode", required=True)
    alltypes = sub.add_parser("alltypes")
    alltypes.add_argument("sed_script", type=Path)
    alltypes.add_argument("output", type=Path)
    alltypes.add_argument("inputs", nargs="+", type=Path)
    syscall = sub.add_parser("syscall")
    syscall.add_argument("source", type=Path)
    syscall.add_argument("output", type=Path)
    version = sub.add_parser("version")
    version.add_argument("output", type=Path)
    version.add_argument("value")
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    if args.mode == "alltypes":
        sed = shutil.which("sed")
        if not sed:
            raise SystemExit("sed is required to generate musl alltypes.h")
        with args.output.open("wb") as output:
            subprocess.run([sed, "-f", str(args.sed_script),
                            *map(str, args.inputs)], stdout=output, check=True)
    elif args.mode == "syscall":
        text = args.source.read_text(encoding="utf-8")
        aliases = "".join(line.replace("__NR_", "SYS_", 1)
                          for line in text.splitlines(True) if "__NR_" in line)
        args.output.write_text(text + aliases, encoding="utf-8")
    else:
        args.output.write_text(f'#define VERSION "{args.value}"\n', encoding="utf-8")


if __name__ == "__main__":
    main()
