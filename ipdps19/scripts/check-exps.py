#!/usr/bin/env python3.6

import os
import sys
import subprocess
import argparse
import itertools
import re
import numpy as np
import time

import data
import fi_tools

try:
    homedir = os.environ['HOME']
except:
    print('Env variable HOME is missing')
    sys.exit(1)

parser = argparse.ArgumentParser('Run profiling experiments')
parser.add_argument('-d', '--appdir', help='applications root directory', required=True)
parser.add_argument('-r', '--resdir', help='results directory', required=True)
parser.add_argument('-n', '--nodes', help='number of nodes', type=int, required=True)
parser.add_argument('-p', '--partition', help='partition to run experiments', choices=['echo', 'local', 'debug', 'batch' ], required=True)
parser.add_argument('-w', '--workers', help='number of workers', type=int, required=True)
parser.add_argument('-t', '--tool', help='tool to run', choices=['refine', 'pinfi', 'golden', 'refine-noff'], required=True)
parser.add_argument('-x', '--action', help='action to take', choices=['profile', 'fi'], required=True)
parser.add_argument('-a', '--apps', help='applications to run ( ' + ' | '.join(data.apps) + ' | ALL ) ', nargs='+', required=True)
parser.add_argument('-c', '--config', help='run configuration \n <serial | omp> <nthreads> [all | app | omplib])', nargs='+', required=True)
parser.add_argument('-i', '--input', help='input size', choices=data.inputs, required=True)
parser.add_argument('-s', '--start', help='start trial', type=int, required=True)
parser.add_argument('-e', '--end', help='end trial', type=int, required=True)
parser.add_argument('-v', '--verbose', help='verbose', default=False, action='store_true') 
args = parser.parse_args()

# Error checking
assert os.path.isdir(args.appdir), 'Applications directory: ' + args.appdir + 'does not exist'
assert args.nodes > 0, 'Nodes arg must be > 0'
assert args.workers >= 0, 'Workers must be >= 0'
assert not (args.tool == 'golden' and args.action == 'fi'), 'Cannot fi with tool golden'
if args.apps == ['ALL']:
    args.apps = data.apps
else:
    for a in args.apps:
        assert a in data.apps, 'Application: ' + a + ' is invalid'
assert args.start <= args.end, 'Start must be < end'
assert args.config[0] in ['serial', 'omp']
config = args.config[0]
if config == 'omp':
    if args.tool == 'golden':
        assert len(args.config) == 2, 'Golden config: omp <nthreads>'
        nthreads = args.config[1]
        assert int(nthreads) > 0, 'nthreads must be > 0'
        instrument=''
    else: # refine or pinfi
        assert len(args.config) == 3, 'Config: omp <nthreads> <all | app | omplib>'
        nthreads = args.config[1]
        instrument = args.config[2]
        assert int(nthreads) > 0, 'nthreads must be > 0'
        assert instrument in ['all', 'app', 'omplib']
else: # serial
    assert len(args.config) == 1, 'Serial config has no other argument'
    nthreads = ''
    instrument = ''

print('==== EXPERIMENT ====')
print( 'Experiment: %s %s %s %s %s %s %s %s %s'%( args.tool, config, ' '.join(args.apps), args.action, instrument, nthreads, args.input, args.start, args.end) )
# Create fault injection experiment tuples
exps = []
for trial in range(args.start, args.end+1):
    for app in args.apps:
        basedir = '%s/%s/%s/%s/%s/%s/%s/%s/'%(args.resdir, args.tool, config, app, args.action, instrument, nthreads, args.input)
        
        if args.action == 'fi':
            # get timeout
            profiledir = '%s/%s/%s/%s/%s/%s/%s/%s'%(args.resdir, args.tool, config, app, 'profile', instrument, nthreads, args.input)
            fname = '/%s/mean_time.txt'%(profiledir)
            with open(fname, 'r') as f:
                timeout = float( f.read() )
            # XXX: PINFI instruction counts BB, but faultinjections is INS! Use a large timeout
            #print('Read mean profiling time: %.2f, setting timeout 20x:  %.2f'%(timeout, timeout*20) )
            timeout = round(20 * timeout, 2)

        trialdir = basedir + '/' + str(trial) +'/'
        #print(trialdir)
        # Skip already done experiments
        #print('CHECK to skip %s/ret.txt'%(trialdir) )
        if os.path.isfile(trialdir+'/ret.txt'):
            continue

        # Create executable
        ## Find the program binary to run
        if args.tool == 'pinfi' or args.tool == 'golden':
            compiled = 'golden'
        elif args.tool == 'refine':
            if instrument == 'omplib':
                compiled = 'golden'
            else:
                compiled = 'refine'
        else:
            print('Unknown tool' + args.tool)
            sys.exit(1)
        rootdir = '%s/%s/%s/'%(args.appdir, compiled, config)

        ## Build the exelist
        ### Any tool specific exelist header
        fi_exelist=[]
        if args.tool in fi_tools.exelist:
            if config == 'serial':
                fi_exelist = fi_tools.exelist[args.tool][config][args.action]
            else:
                fi_exelist = fi_tools.exelist[args.tool][config][args.action][instrument]
        #print(fi_exelist)

        ### Patch APPDIR and NTHREADS if needed
        # XXX: replace $APPDIR, needed only for CoMD
        exelist = [ s.replace( '$APPDIR', '%s/%s'%(rootdir, app) ) for s in data.programs[config][app]['exec'][args.input] ]
        # XXX: replace NTHREADS, needed only for XSBench omp
        exelist = [ s.replace( '$NTHREADS', nthreads ) for s in exelist ]
        # XXX: note using rootdir, program binary in data is relative to that
        exelist = fi_exelist + [ rootdir + exelist[0] ] + exelist[1:]
        exelist = ' '.join(exelist)
        #print('\nexelist< %s >\n'%(exelist))

        ## Setup the env
        runenv = []
        if config == 'omp':
            runenv += [ '-e', 'OMP_NUM_THREADS', nthreads ]
            if int(nthreads) > 1:
                runenv += [ '-e', 'KMP_AFFINITY' , 'compact' ]
        if args.tool == 'refine':
            if instrument == 'all' or instrument == 'omplib':
                runenv += [ '-e', 'LD_LIBRARY_PATH', homedir + '/usr/local/refine/lib:' + homedir + '/usr/local/lib' ]

        ## Append to experiments (must be string list)
        exps.append( runenv + ['-r', trialdir, str(timeout), exelist] )
        if args.verbose:
            print(runenv + ['-r', trialdir, str(timeout), exelist])
        #sys.exit(123)
#print("==== experiments ====")
#print(exps)
#print("==== end experiments ====")
print('Nof exps: %d'%( len(exps) ) )

print('\nExiting bye-bye')
print('==== END EXPERIMENT ====')

