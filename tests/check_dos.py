#!/usr/bin/env python3
"""Boot a disposable copy of a supplied DOS image; never distribute DOS itself."""
import argparse
import pathlib
import shutil
import subprocess
from cases import CASES

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--image', type=pathlib.Path, required=True)
parser.add_argument('--compiler', type=pathlib.Path, default=pathlib.Path('build/c2bat'))
parser.add_argument('--work', type=pathlib.Path, required=True, help='new directory for image and serial log')
args = parser.parse_args()
compiler = args.compiler.resolve()
args.work.mkdir(parents=True, exist_ok=False)
image = args.work / 'boot.img'
shutil.copyfile(args.image, image)

def copy(source, destination):
    subprocess.run(['mcopy', '-o', '-i', str(image), str(source), '::' + destination], check=True)

def guest_file(name, data):
    path = args.work / name
    path.write_bytes(data)
    copy(path, name)

commands = ['@ECHO OFF', 'CTTY AUX', 'VER']
for index, (name, source, expected) in enumerate(CASES):
    cfile = args.work / f'{name}.c'
    cfile.write_text(source)
    output = args.work / name
    subprocess.run([str(compiler), str(cfile), '-o', str(output)], check=True)
    if index == 0:
        for path in output.glob('*.BAT'):
            if path.name != 'RUN.BAT':
                copy(path, path.name)
    batch = f'T{index}.BAT'
    copy(output / 'RUN.BAT', batch)
    commands += [f'ECHO BEGIN_{name}', f'CALL {batch}']
    if expected is None:
        commands += [f'IF "%CBERR%"=="RANGE" ECHO PASS_{name}']
    else:
        commands += [f'IF "%CBERR%"=="" IF "%CBRESULT%"=="{expected}" ECHO PASS_{name}']
commands += ['ECHO SUITE_DONE', 'QEXIT.COM']
guest_file('AUTOEXEC.BAT', ('\r\n'.join(commands) + '\r\n').encode('ascii'))
guest_file('CONFIG.SYS', b'SHELL=A:\\COMMAND.COM A:\\ /E:4096 /P\r\nFILES=20\r\nBUFFERS=10\r\n')
# Test harness only: OUT 0xf4,0x10 followed by DOS process exit.
# The generated program/runtime itself consists exclusively of batch files.
guest_file('QEXIT.COM', bytes.fromhex('ba f4 00 b8 10 00 ef b8 00 4c cd 21'))
command = ['qemu-system-i386', '-display', 'none', '-monitor', 'none',
           '-machine', 'pc', '-cpu', '486', '-m', '4',
           '-drive', f'if=floppy,format=raw,file={image},cache=writethrough',
           '-boot', 'a', '-serial', 'stdio', '-no-reboot',
           '-device', 'isa-debug-exit,iobase=0xf4,iosize=0x04']
try:
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=120)
except subprocess.TimeoutExpired as error:
    (args.work / 'serial.log').write_bytes(error.stdout or b'')
    raise SystemExit(f'DOS timed out; inspect {args.work / "serial.log"}')
log = result.stdout.decode('ascii', errors='replace')
(args.work / 'serial.log').write_text(log)
print(log)
missing = [name for name, _, _ in CASES if f'PASS_{name}' not in log]
if result.returncode != 33 or missing or 'SUITE_DONE' not in log:
    raise SystemExit(f'DOS check failed: exit={result.returncode}, missing={missing}')
print(f'PASS: {len(CASES)} programs under booted COMMAND.COM; log: {args.work / "serial.log"}')
