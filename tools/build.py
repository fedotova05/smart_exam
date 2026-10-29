"""Дополнительная сборка без IDE; основные проекты находятся в project/."""
import argparse
import os
from pathlib import Path
import subprocess
from concurrent.futures import ThreadPoolExecutor

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
ui_sources = [p for p in sources if Path(p).parts[1] in ('ui', 'graphics')]
sources = [p for p in sources if p not in ui_sources]
sqlite_object = out / 'sqlite3.o'
compiler_stamp = out / 'compiler.txt'
compiler_id = str(Path(args.compiler).resolve()) + '\n' + subprocess.check_output([args.compiler, '--version'], text=True)
if not sqlite_object.exists() or sqlite_object.stat().st_mtime < Path('third_party/sqlite/sqlite3.c').stat().st_mtime or not compiler_stamp.exists() or compiler_stamp.read_text() != compiler_id:
    subprocess.run([str(Path(args.compiler).with_name('gcc.exe')), '-O2', '-D_WIN32_WINNT=0x0600', '-c', 'third_party/sqlite/sqlite3.c', '-o', str(sqlite_object)], check=True)
    compiler_stamp.write_text(compiler_id)
tests = [str(p) for p in sorted(Path('tests').rglob('*.cpp')) if 'ui' not in p.parts]
ui_tests = [str(p) for p in sorted(Path('tests/ui').glob('*.cpp'))]
suffix = '-s-d' if args.mode == 'Debug' else '-s'
graphics_flags = ['-L' + args.sfml + '/lib']
graphics_flags += ['-lsfml-' + name + suffix for name in ('graphics', 'window', 'system')]
graphics_flags += ['-lopengl32', '-lfreetype', '-lwinmm', '-lgdi32', '-lcomdlg32']
headers_time = max(p.stat().st_mtime for folder in ('src', 'tests') for p in Path(folder).rglob('*.h'))

def compile_source(source):
    output = out / 'objects' / (source.replace('\\', '/').replace('/', '_') + '.o')
    output.parent.mkdir(exist_ok=True)
    command = [args.compiler] + flags
    if source in ui_sources + ui_tests + ['main.cpp']:
        command += ['-DSFML_STATIC', '-I' + args.sfml + '/include']
    command += ['-c', source, '-o', str(output)]
    stamp = output.with_suffix('.command')
    signature = repr(command) + compiler_id
    if not output.exists() or output.stat().st_mtime < max(Path(source).stat().st_mtime, headers_time) or not stamp.exists() or stamp.read_text() != signature:
        subprocess.run(command, check=True)
        stamp.write_text(signature)
    return str(output)

all_sources = sources + ui_sources + tests + ui_tests + ['main.cpp']
with ThreadPoolExecutor(max_workers=4) as pool:
    objects = dict(zip(all_sources, pool.map(compile_source, all_sources)))

def link(name, files, graphics=False):
    command = [args.compiler] + [objects[source] for source in files] + [str(sqlite_object)]
    if graphics:
        command += graphics_flags
    subprocess.run(command + ['-o', str(out / (name + '.exe'))], check=True)

link('tests', sources + tests)
subprocess.run([str(out / 'tests.exe')], check=True)
link('smart_exam', sources + ui_sources + ['main.cpp'], True)
if ui_tests:
    link('ui_tests', sources + ui_sources + ui_tests, True)
    subprocess.run([str(out / 'ui_tests.exe')], check=True, timeout=60)
if args.smoke:
    subprocess.run([str(out / 'smart_exam.exe'), '--smoke'], check=True, timeout=15)
subprocess.run([str(out / 'smart_exam.exe'), '--demo'], check=True, timeout=15)
