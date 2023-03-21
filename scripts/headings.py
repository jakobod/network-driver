#!/usr/bin/env python3
import argparse
from os import walk, path


def get_header(file_name):
    return [
        '/**\n',
        ' *  @author    Jakob Otto\n',
        f' *  @file      {file_name}\n',
        ' *  @copyright Copyright 2023 Jakob Otto. All rights reserved.\n',
        ' *             This file is part of the network-driver project, released under\n',
        ' *             the GNU GPL3 License.\n',
        ' */\n']


def find_end_comment(lines):
    for i, l in enumerate(lines):
        if l.startswith(' */'):
            return i
    return -1


def remove_header(end, lines):
    return lines[end + 1:]


def change_header(file_path):
    lines = []
    print(file_path)
    with open(file_path, 'r') as f:
        lines = f.readlines()
        if lines[0].startswith('/**'):
            lines = lines[find_end_comment(lines) + 1:]
    with open(file_path, 'w') as of:
        of.writelines(get_header(path.basename(file_path)) + lines)


def tree(dir):
    for (dirpath, dirnames, filenames) in walk(dir):
        for f in filenames:
            if f.endswith('.cpp') or f.endswith('.hpp'):
                change_header(f'{dir}/{f}')
        for d in dirnames:
            tree(f'{dir}/{d}')
        break


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--dir')

    args = parser.parse_args()

    tree(args.dir)


if __name__ == '__main__':
    main()
