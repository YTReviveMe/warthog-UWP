#!/usr/bin/env python3
import re
import sys
from pathlib import Path


def main() -> None:
    guest_source, output_path = map(Path, sys.argv[1:3])
    source = guest_source.read_text(encoding="utf-8")
    declarations = re.findall(
        r"^([A-Za-z_][A-Za-z0-9_ \t*]*?)\s+(hostposix_(\w+))\((.*?)\);$",
        source,
        re.M,
    )
    generated = [
        '#include "host.h"',
        '#include "posix.h"',
        '#include "browser_http.h"',
        "#include <windows.h>",
        "#include <stdint.h>",
        "",
        "#define DIRECTORY_HANDLE_COUNT 256",
        "static void *directory_handles[DIRECTORY_HANDLE_COUNT];",
        "static SRWLOCK directory_handle_lock = SRWLOCK_INIT;",
        "",
        "static uint32_t directory_handle_new(void *directory)",
        "{",
        "    uint32_t index;",
        "",
        "    if (!directory)",
        "        return 0;",
        "    AcquireSRWLockExclusive(&directory_handle_lock);",
        "    for (index = 1; index < DIRECTORY_HANDLE_COUNT && directory_handles[index]; index++)",
        "        ;",
        "    if (index < DIRECTORY_HANDLE_COUNT)",
        "        directory_handles[index] = directory;",
        "    ReleaseSRWLockExclusive(&directory_handle_lock);",
        "    if (index == DIRECTORY_HANDLE_COUNT)",
        "        posix_directory_close(directory);",
        "    return index < DIRECTORY_HANDLE_COUNT ? index : 0;",
        "}",
        "",
        "static void *directory_handle_get(void *handle)",
        "{",
        "    uintptr_t index = (uintptr_t)handle;",
        "",
        "    return index && index < DIRECTORY_HANDLE_COUNT ? directory_handles[index] : NULL;",
        "}",
        "",
        "static void *directory_handle_take(void *handle)",
        "{",
        "    uintptr_t index = (uintptr_t)handle;",
        "    void *directory = NULL;",
        "",
        "    AcquireSRWLockExclusive(&directory_handle_lock);",
        "    if (index && index < DIRECTORY_HANDLE_COUNT)",
        "    {",
        "        directory = directory_handles[index];",
        "        directory_handles[index] = NULL;",
        "    }",
        "    ReleaseSRWLockExclusive(&directory_handle_lock);",
        "    return directory;",
        "}",
        "",
    ]
    for return_type, host_name, suffix, parameters in declarations:
        parameter_list = (
            [] if parameters.strip() in ("", "void") else [item.strip() for item in parameters.split(",")]
        )
        named_parameters = []
        arguments = []
        for index, parameter in enumerate(parameter_list):
            argument = f"a{index}"
            match = re.match(r"^(.*?)([A-Za-z_]\w*)$", parameter)
            parameter_type = match.group(1).rstrip() if match else parameter
            named_parameters.append(f"{parameter_type} {argument}")
            arguments.append(argument)

        body = f"    {'return ' if return_type != 'void' else ''}posix_{suffix}({', '.join(arguments)});"
        if suffix == "directory_open":
            body = "    return (void *)(uintptr_t)directory_handle_new(posix_directory_open(a0));"
        elif suffix == "directory_next":
            body = "    return posix_directory_next(directory_handle_get(a0), a1, a2);"
        elif suffix == "directory_close":
            body = ("    void *directory = directory_handle_take(a0);\n"
                    "    if (directory) posix_directory_close(directory);")
        generated += [
            f"{return_type} GUEST_ABI {host_name}({', '.join(named_parameters) or 'void'})",
            "{",
            body,
            "}",
            "",
        ]
    output_path.write_text("\n".join(generated), encoding="utf-8")


if __name__ == "__main__":
    main()
