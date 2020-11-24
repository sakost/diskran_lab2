#!/usr/bin/env python
import argparse
import contextlib
import json
import os
import pathlib
import subprocess
import sys
from functools import partial
import random
import string
import tempfile
import shutil


def generate_key():
    return ''.join(random.choices(string.ascii_lowercase[:5] + string.ascii_uppercase[:5], k=random.randint(1, 3)))


def generate_test(tmp, rows, percent):
    ans = []
    percent *= rows
    percent = int(percent)
    saved_files = set()
    for i in range(rows-percent):
        choice = random.choice(['+', '-', '?'])
        if random.random() > 0.8 and percent > 0:
            name = f"db{len(saved_files)}"
            name = os.path.join(tmp, name)
            ans.append(f'! Save {name}')
            saved_files.add(name)
            percent -= 1
        elif random.random() > 0.8 and len(saved_files) > 0:
            ans.append(f'! Load {saved_files.pop()}')
        if choice == '?':
            ans.append(generate_key())
        elif choice == '+':
            ans.append('+ ' + generate_key() + ' ' + str(random.randint(0, 2**64 - 1)))
        else:
            ans.append('- ' + generate_key())

    for el in saved_files:
        ans.append(f'! Load {el}')
    for i in range(percent):
        name = f'db{i}'
        name = os.path.join(tmp, name)
        ans.append(f'! Save {name}')
        ans.append(f'! Load {name}')
    return ans


def solve_problem(test):
    d = {}
    for r in test:
        r = r.lower()
        if r[0] == '!':
            cmd, filename = r[2:].split()
            if cmd == 'load':
                with open(filename, 'r') as file:
                    d = json.load(file)
            elif cmd == "save":
                with open(filename, 'w') as file:
                    json.dump(d, file)
            print('OK')
        elif r[0] == '+':
            key, value = r[2:].split()
            if key in d:
                print('Exist')
            else:
                d[key] = int(value)
                print('OK')
        elif r[0] == '-':
            key = r[2:]
            if key not in d:
                print('NoSuchWord')
            else:
                del d[key]
                print('OK')
        else:
            if r in d:
                print('OK:', d[r])
            else:
                print('NoSuchWord')


def main():
    # with open('data.txt', 'wb') as file:
    #     test = generate_test(os.path.curdir, 150, 0.1)
    #     file.write(('\n'.join(test)).encode())
    #
    # return
    parser = argparse.ArgumentParser()

    parser.add_argument('prog', default=partial(pathlib.Path, 'cmake-build-debug/lab2'), type=pathlib.Path)
    parser.add_argument('-R', '--random-state', default=42, type=int)
    parser.add_argument('-n', '--tests-count', default=1000, type=int)
    parser.add_argument('-N', '--rows-count', default=150, type=int)
    parser.add_argument('-p', '--percent-load-save', default=0.01, type=float)

    args = parser.parse_args(sys.argv[1:])

    tmp_dir = tempfile.mkdtemp()

    for i in range(args.tests_count):
        random.seed(args.random_state)
        test = generate_test(tmp_dir, args.rows_count, args.percent_load_save)
        test_myself = test.copy()
        test_myself = list(map(lambda x: x.replace('db', 'dbother'), test_myself))
        f_myself_out = open(os.path.join(tmp_dir, 'my_out.txt'), 'w+')
        with contextlib.redirect_stdout(f_myself_out):
            solve_problem(test_myself)
        f_myself_out.close()

        proc = subprocess.Popen(args.prog.as_posix(), stdout=subprocess.PIPE, stdin=subprocess.PIPE, shell=True)
        proc.stdin.write(('\n'.join(test)).encode())
        proc.stdin.close()
        proc.wait()
        if proc.returncode != 0:
            print('some error:', proc.returncode)
            print(*test, sep='\n', file=open(os.path.join(os.path.dirname(__file__), 'test.txt'), 'w'))
            break
        out = proc.stdout.read()

        with open(os.path.join(tmp_dir, 'out.txt'), 'wb+') as file:
            file.write(out)

        code = subprocess.Popen(['diff', '-w', os.path.join(tmp_dir, 'out.txt'), os.path.join(tmp_dir, 'my_out.txt')],
                                shell=False, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        code.wait()
        errors = code.stdout.read().decode()
        if errors:
            print(errors)
            print(*test, sep='\n', file=open(os.path.join(os.path.dirname(__file__), 'test.txt'), 'w'))
            break

    shutil.rmtree(tmp_dir)


if __name__ == '__main__':
    main()
