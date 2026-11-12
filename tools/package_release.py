"""Собирает переносимый выпуск из проверенной Release-сборки."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import zipfile

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--compiler-dir', default='C:/Program Files/CodeBlocks/MinGW/bin')
parser.add_argument('--sfml', default='C:/SFML-2.5.1')
args = parser.parse_args()
output = root / 'packaging/output'
output.mkdir(parents=True, exist_ok=True)
stage = Path(tempfile.mkdtemp(prefix='SmartExam-', dir=output)).resolve()
assert output.resolve() in stage.parents
binary = root / 'bin/Release/smart_exam.exe'
if not binary.is_file():
    raise SystemExit('Сначала выполните python tools/build.py --mode Release')
shutil.copy2(binary, stage / binary.name)
for directory in ('db', 'docs'):
    shutil.copytree(root / directory, stage / directory)
for directory in ('oge', 'samples'):
    shutil.copytree(root / 'data' / directory, stage / 'data' / directory)
for name in ('README.md',):
    shutil.copy2(root / name, stage / name)
license_dir = stage / 'licenses'
license_dir.mkdir()
shutil.copytree(root / 'third_party/resvg', stage / 'third_party/resvg')
shutil.copy2(root / 'third_party/sqlite/README.md', license_dir / 'sqlite.txt')
sfml_license = Path(args.sfml) / 'license.txt'
if sfml_license.is_file():
    shutil.copy2(sfml_license, license_dir / 'sfml.txt')
compiler = Path(args.compiler_dir)
pending = [stage / binary.name]
copied = set()
while pending:
    file = pending.pop()
    text = subprocess.check_output([str(compiler / 'objdump.exe'), '-p', str(file)], text=True)
    for dll in re.findall(r'DLL Name:\s*(\S+)', text):
        if dll.lower() in copied:
            continue
        copied.add(dll.lower())
        source = next((p / dll for p in (compiler, Path(args.sfml) / 'bin') if (p / dll).is_file()), None)
        if source:
            shutil.copy2(source, stage / dll)
            pending.append(stage / dll)
(stage / 'start.bat').write_text('@echo off\nchcp 65001 >nul\ncd /d "%~dp0"\nsmart_exam.exe\n', encoding='utf-8')
environment = os.environ.copy()
environment['PATH'] = str(Path(os.environ['SystemRoot']) / 'System32')
for option in ('--demo', '--smoke'):
    subprocess.run([str(stage / binary.name), option], cwd=stage, env=environment, check=True, timeout=30)
for name in ('smart_exam.db', 'smart_exam.db-wal', 'smart_exam.db-shm'):
    (stage / 'data' / name).unlink(missing_ok=True)
manifest = []
for file in sorted(stage.rglob('*')):
    if file.is_file():
        manifest.append(hashlib.sha256(file.read_bytes()).hexdigest() + '  ' + file.relative_to(stage).as_posix())
(stage / 'SHA256SUMS.txt').write_text('\n'.join(manifest) + '\n', encoding='utf-8')
archive = output / 'SmartExam-Windows.zip'
with zipfile.ZipFile(archive, 'w', compression=zipfile.ZIP_DEFLATED) as zipped:
    for file in sorted(stage.rglob('*')):
        if file.is_file():
            entry = zipfile.ZipInfo('SmartExam/' + file.relative_to(stage).as_posix(), (2026, 1, 1, 0, 0, 0))
            entry.compress_type = zipfile.ZIP_DEFLATED
            zipped.writestr(entry, file.read_bytes())
digest = hashlib.sha256(archive.read_bytes()).hexdigest()
archive.with_suffix('.zip.sha256').write_text(digest + '  ' + archive.name + '\n', encoding='ascii')
print('Release:', archive)
print('Folder:', stage)
print('SHA256:', digest)
