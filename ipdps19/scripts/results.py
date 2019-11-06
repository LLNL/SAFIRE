#!/usr/bin/env python3.6

import os
import sys
import subprocess
import argparse
import itertools
import re
import numpy as np
import time
import analysis

import data
import fi_tools

parser = argparse.ArgumentParser('Run profiling experiments')
parser.add_argument('-r', '--resdir', help='results directory', required=True)
parser.add_argument('-t', '--tool', help='tool to run', choices=['safire', 'refine', 'pinfi', 'pinfi-detach', 'golden' ], required=True)
parser.add_argument('-x', '--action', help='action to check results for', choices=['profile', 'fi', 'fi-0', 'fi-1-15'], required=True)
parser.add_argument('-c', '--config', help='run configuration \n <serial | omp> <nthreads> <all | app | omplib>)', nargs='+', required=True)
parser.add_argument('-a', '--apps', help='applications to run ( ' + ' | '.join(data.apps) + ' | ALL ) ', nargs='+', required=True)
parser.add_argument('-i', '--input', help='input size', choices=data.inputs, required=True)
parser.add_argument('-s', '--start', help='start trial', type=int, required=True)
parser.add_argument('-e', '--end', help='end trial', type=int, required=True)
parser.add_argument('-v', '--verbose', help='verbose', default=False, action='store_true') 
parser.add_argument('-w', '--wait', help='wait policy', choices=['passive', 'active', ''], required=True)
args = parser.parse_args()

# Error checking
assert os.path.isdir(args.resdir), 'Results directory: ' + args.resdir + 'does not exist'
for a in args.apps:
    assert a in data.apps or a == 'ALL', 'Application: ' + a + ' is invalid'
if args.apps == ['ALL']:
    args.apps = data.apps
assert args.start <= args.end, 'Start must be < end'
assert not (args.tool == 'golden' and args.action == 'fi'), 'Cannot fi with tool golden'
assert args.config[0] in ['serial', 'omp']
config = args.config[0]
if config == 'omp':
    if args.tool == 'golden':
        assert len(args.config) == 2, 'Golden config: omp <nthreads>'
        nthreads = args.config[1]
        assert int(nthreads) > 0, 'nthreads must be > 0'
        instrument=''
    else: # safire, refine or pinfi
        assert len(args.config) == 3, 'Config: omp <nthreads> <all | app | omplib>'
        nthreads = args.config[1]
        instrument = args.config[2]
        assert int(nthreads) > 0, 'nthreads must be > 0'
        assert instrument in ['all', 'app', 'omplib']
else: # serial
    assert len(args.config) == 1, 'Serial config has no other argument'
    nthreads = ''
    instrument = ''

for app in args.apps:
    #def results(resdir, tool, config, app, action, instrument, nthreads, inputsize, start, end, verbose):
    analysis.results(args.resdir, args.tool, config, args.wait, app, args.action, instrument, nthreads, args.input, args.start, args.end, args.verbose)
