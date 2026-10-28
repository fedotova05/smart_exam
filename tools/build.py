"""Дополнительная сборка без IDE; основные проекты находятся в project/."""
import argparse
import os
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--compiler', default='C:/Program Files/CodeBlocks/MinGW/bin/g++.exe')
parser.add_argument('--sfml', default='C:/SFML-2.5.1')
parser.add_argument('--mode', choices=['Debug', 'Release'], default='Debug')
parser.add_argument('--smoke', action='store_true')
args = parser.parse_args()
os.chdir(root)
os.environ['PATH'] = str(Path(args.compiler).parent) + os.pathsep + str(Path(args.sfml) / 'bin') + os.pathsep + os.environ['PATH']
out = root / 'bin' / args.mode
out.mkdir(parents=True, exist_ok=True)
flags = ['-std=c++11', '-Wall', '-Wextra', '-pedantic', '-Isrc', '-Ithird_party/sqlite', '-finput-charset=UTF-8', '-fexec-charset=UTF-8']
flags += ['-g', '-O0'] if args.mode == 'Debug' else ['-O2', '-DNDEBUG']
sources = [str(p) for p in sorted(Path('src').rglob('*.cpp'))]
sqlite_object = out / 'sqlite3.o'
compiler_stamp = out / 'compiler.txt'
compiler_id = str(Path(args.compiler).resolve()) + '\n' + subprocess.check_output([args.compiler, '--version'], text=True)
if not sqlite_object.exists() or sqlite_object.stat().st_mtime < Path('third_party/sqlite/sqlite3.c').stat().st_mtime or not compiler_stamp.exists() or compiler_stamp.read_text() != compiler_id:
    subprocess.run([str(Path(args.compiler).with_name('gcc.exe')), '-O2', '-D_WIN32_WINNT=0x0600', '-c', 'third_party/sqlite/sqlite3.c', '-o', str(sqlite_object)], check=True)
    compiler_stamp.write_text(compiler_id)
sources += [str(sqlite_object)]
tests = [str(p) for p in sorted(Path('tests').rglob('*.cpp'))]
subprocess.run([args.compiler] + flags + sources + tests + ['-o', str(out / 'tests.exe')], check=True)
subprocess.run([str(out / 'tests.exe')], check=True)
subprocess.run([args.compiler] + flags + sources + ['main.cpp', '-I' + args.sfml + '/include', '-L' + args.sfml + '/lib', '-lsfml-graphics', '-lsfml-window', '-lsfml-system', '-o', str(out / 'smart_exam.exe')], check=True)
if args.smoke:
    subprocess.run([str(out / 'smart_exam.exe'), '--smoke'], check=True, timeout=15)
