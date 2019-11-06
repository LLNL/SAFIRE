#!/usr/bin/env python

import subprocess
import timeit
import sys
import data
import argparse
import os
import numpy as np

def make(workdir, args):
    out = open(workdir+'/compile-out.txt', 'w')
    err = open(workdir+'/compile-err.txt', 'w')
    p = subprocess.Popen(['make'] + args, stdout = out, stderr = err, cwd = workdir)
    p.wait()
    if p.returncode == 0:
        print('make succeeded')
    else:
        print('make failed!')
        sys.exit(p.returncode)
    out.close()
    err.close()

def clean(workdir):
    p = subprocess.Popen(['make', 'clean'], cwd = workdir, stdout = None)
    p.wait()
    if p.returncode == 0:
        print('make clean succeeded')
    else:
        print('make clean failed!')
        sys.exit(p.returncode)

    # XXX: veryclean is not supported by all, run opportunistically
    p = subprocess.Popen(['make', 'veryclean'], cwd = workdir, stdout = None)
    p.wait()

def build(action, repeat, workdir, args):
    if action == 'clean':
        clean(workdir)
    elif action == 'build':
        make(workdir, args)
    elif action == 'profile':
        clean(workdir)
        times = []
        for r in range(1,repeat+1):
            t = timeit.timeit(lambda: make(workdir, args), number = 1)
            times.append(t)
            with open(workdir+'compile-time-' + str(r) + '.txt', 'w') as f:
                f.write('%.2f'%(t) + '\n')
            clean(workdir)
        with open(workdir+'mean-compile-time.txt', 'w') as f:
            f.write('%.2f'%(np.mean(times)) + '\n')

parser = argparse.ArgumentParser('Build applications for tools and their configurations')
# tuple of 3: (tool, app, iter)
parser.add_argument('-d', '--appdir', help='application directory', required=True)
parser.add_argument('-t', '--tools', help='tool to build', choices=['golden','safire','refine'], nargs='+', required=True)
parser.add_argument('-c', '--configs', help='configuration to build', choices=['serial','omp'], nargs='+', required=True)
parser.add_argument('-a', '--apps', help='applications to build ( ' + ' | '.join(data.apps) + ' | ALL ) ', nargs='+', required=True)
parser.add_argument('-i', '--inputs', help='inputs to build for (' + ' | '.join(data.inputs) + ' )', nargs='+', required=True)
parser.add_argument('-o', '--action', help='action', choices=['build','clean','profile'], required=True)
parser.add_argument('-r', '--repeat', help='repeat build action ( to profile compiling times )', type=int, default=1)
args = parser.parse_args()

assert os.path.isdir(args.appdir), 'Application directory: ' + args.appdir + 'does not exist'
for a in args.apps:
    assert a in data.apps or a == 'ALL', 'Application: ' + a + ' is invalid'
if args.apps == ['ALL']:
    args.apps = data.apps
for i in args.inputs:
    assert i in data.inputs, 'Input:' + i + ' is invalid'
assert args.repeat >=1, 'Repeat: ' + args.repeat+ 'is invalid'

for t in args.tools:
    for c in args.configs:
        for a in args.apps:
            for i in args.inputs:
                print('%s %s %s %s'%(t,c,a,i))
                workdir = args.appdir+'/'+t+'/'+c+'/'+data.programs[c][a]['builddir']
                build_args = data.programs[c][a]['buildargs'][i]
                print('Build dir: '+workdir+' buildargs '+' '.join(build_args))

                build(args.action, args.repeat, workdir, build_args)

