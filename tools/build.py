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
flags = ['-std=c++11', '-Wall', '-Wextra', '-pedantic', '-Isrc', '-finput-charset=UTF-8', '-fexec-charset=UTF-8']
flags += ['-g', '-O0'] if args.mode == 'Debug' else ['-O2', '-DNDEBUG']
sources = [str(p) for p in sorted(Path('src').rglob('*.cpp'))]
tests = [str(p) for p in sorted(Path('tests').rglob('*.cpp'))]
subprocess.run([args.compiler] + flags + sources + tests + ['-o', str(out / 'tests.exe')], check=True)
subprocess.run([str(out / 'tests.exe')], check=True)
subprocess.run([args.compiler] + flags + sources + ['main.cpp', '-I' + args.sfml + '/include', '-L' + args.sfml + '/lib', '-lsfml-graphics', '-lsfml-window', '-lsfml-system', '-o', str(out / 'smart_exam.exe')], check=True)
if args.smoke:
    subprocess.run([str(out / 'smart_exam.exe'), '--smoke'], check=True, timeout=15)
