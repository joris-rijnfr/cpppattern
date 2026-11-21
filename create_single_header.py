import os
import re


entry_path = ["src", "matcher.hpp"]
output_path = ["cpppattern.hpp"]
version = "0.1.0"

content = f"""// Copyright (c) 2026 Joris Rijnfrank
// Licensed under the MIT License.
// See https://github.com/joris-rijnfr/cpppattern/blob/main/LICENSE

// Version: {version}
// Source: https://github.com/joris-rijnfr/cpppattern

#pragma once
"""


def main():
    include_file(entry_path)
    print(f"Writing single header file at {os.sep.join(output_path)}...")
    with open(os.sep.join(output_path), "w") as output:
        output.write(content)
    print("Done!")


already_included = set()
def include_file(path):
    global content
    path_str = os.sep.join(path)
    if not path[-1].endswith(".hpp") or path_str in already_included:
        return
    print(f"Adding file {path_str}...")
    already_included.add(path_str)
    content += "\n// Begin " + path_str + "\n"
    with open(path_str, "r") as file:
        for line in file:
            handle_line(line, path)
    content += "// End " + path_str + "\n\n"


in_do_not_include_block = False
def handle_line(line, path):
    global content, in_do_not_include_block
    stripped_line = line.strip()
    if stripped_line == "/*** BEGIN - NOT IN SINGLE HEADER ***/":
        in_do_not_include_block = True
        return
    if stripped_line == "/*** END - NOT IN SINGLE HEADER ***/":
        in_do_not_include_block = False
        return
    if in_do_not_include_block or stripped_line == "" or stripped_line == "#pragma once":
        return
    result_include = re.search("^#include\\s+\"(.+)\"$", stripped_line)
    if result_include != None:
        include_file(join_paths(path, result_include.group(1)))
        return
    content += line


def join_paths(path1, path2_str):
    path1.pop()
    path2 = path2_str.split(os.sep)
    for segment in path2:
        if segment == "..":
            if len(path1) == 0:
                raise IndexError("An include cannot reference a file outside of the project root directory.")
            path1.pop()
        elif segment != ".":
            path1.append(segment)
    return path1


if __name__ == "__main__":
    main()
