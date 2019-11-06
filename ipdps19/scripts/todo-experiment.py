#!/usr/bin/env python3

import argparse
import re
import os
import collections
import json
import numpy as np
import scipy.stats as stats
import glob
import sys

import data
import fi_tools
import config

# Parse profiling files
def parse_profiles(resdir, tool, config, wait, app, instrument, nthreads, inputsize, start, end):
    inscount = []
    if config == 'omp':
        thread_inscount = {}
        for i in range(0, int(nthreads) ):
            thread_inscount[i] = []
    xtime = []
    
    #print(tool) # ggout
    # Parse inscounts for tools other than golden
    if tool == 'golden':
        do_inscount = False
    else:
        do_inscount = True 

    profiledir = '%s/%s/%s/%s/%s/%s/%s/%s/%s'%(resdir, tool, config, wait, app, 'profile', instrument, nthreads, inputsize)

    files = glob.glob(profiledir + '/*/ret.txt')
    return ( len(files) < ( end - start + 1 ) )
    #print( profiledir ) # ggout

    #pending = False
    #for trial in range(start, end+1):
    #    trialdir = '%s/%s/'%(profiledir, trial)

    #    # Check if it has ran
    #    try:
    #        with open(trialdir + '/ret.txt') as f:
    #            pass
    #    except FileNotFoundError:
    #        pending = True
    #        break
    #    # Check inscount
    #    if do_inscount:
    #        fname = trialdir + fi_tools.files[tool]['inscount']
    #        try:
    #            #if not os.path.isfile( fname ):
    #            with open(fname, 'r') as f:
    #                pass
    #        except FileNotFoundError:
    #            print('Missing %s'%( fname ) )
    #            if os.path.isfile( trialdir + '/ret.txt' ):
    #                os.remove( trialdir + '/ret.txt' )
    #            pending = True
    #            break

    #return pending
    
def analyze_profiles(results, resdir, tools, apps, start, end):
    # fi tools
    for t in tools:
        for c in config.data[t]['exec']:
            for w in config.data[t][c]['wait']:
                pending_apps = set()
                for a in apps:
                    for i in config.data[t][c]['inputs']:
                        for ins in config.data[t][c]['instrument']:
                            for n in config.data[t][c][i][ins]['nthreads']:
                                #print('\r', '<%8s> Processing %8s %8s %8s %8s %5s %3s %6s' % ('profile', t, a, c, w, i, n, ins), end='' )
                                print('<%8s> Processing %8s %8s %8s %8s %5s %3s %6s' % ('profile', t, a, c, w, i, n, ins) )
                                profiledir = '%s/%s/%s/%s/%s/%s/%s/%s/%s'%(resdir, t, w, c, a, 'profile', ins, n, i)
                                pending = parse_profiles(resdir, t, c, w, a, ins, n, i, start, end)
                                if pending:
                                    pending_apps.add( a )
                if pending_apps:
                    with open('todo-%s.sh'%( t ), 'a') as f:
                        runstr = '../sched.py -d $PROGDIR -r $RESDIR -a %s -t %s -x %s -s %s -e %s -c %s -l 1800 -N 4 -p batch -g' % (
                                ' '.join( pending_apps ), t, 'profile', start, end, c )
                        if w != '':
                            runstr += ' -w %s' % ( w )
                        runstr += '\n'
                        f.write( runstr )

# Parse fault injection files
def parse_fi(resdir, tool, config, wait, app, action, instrument, nthreads, inputsize, start, end, verbose):
    print('===== APP %s %s %s %s %s %s %s %s %s  ====='%(app, tool, config, action, instrument, nthreads, inputsize, start, end) )
    missing = timeout = crash = soc = benign = 0

    basedir = '%s/%s/%s/%s/%s/%s/%s/%s/%s/'%(resdir, tool, config, wait, app, action, instrument, nthreads, inputsize)

    #files = glob.glob(basedir + '/*/ret.txt' )
    #return ( len( files ) < ( end - start + 1 ) )
    #xtime = []
    pending = False
    for trial in range(start, end+1):
        trialdir = '%s/%s/'%( basedir, trial )

        try:
            with open(trialdir + '/ret.txt', 'r') as f:
                out = f.read().strip()
                if out == 'timeout':
                    print('TIMEOUT trialdir %s'%( trialdir ) )
                    os.remove(trialdir + '/ret.txt')
                    pending = True
        except FileNotFoundError:
            print('PENDING trialdir %s'%( trialdir ) )
            pending = True
            continue

        try:
            with open(trialdir + fi_tools.files[tool]['injection'], 'r'):
                pass
        except FileNotFoundError:
            print('MISSING %s' % (trialdir)  )
            pending = True
            try:
                os.remove(trialdir + '/ret.txt')
                os.remove(trialdir + fi_tools.files[tool]['target'] )
            except OSError:
                pass
        
    return pending


def analyze_fi(results, resdir, tools, apps, start, end):
    for t in tools:
        for c in config.data[t]['exec']:
            for w in config.data[t][c]['wait']:
                pending_apps = set()
                for a in apps:
                    for ins in config.data[t][c]['instrument']:
                        # XXX: REMOVE side-effect of active missing, think about moving to config data if used
                        #fi_targets = ['fi']
                        #if t == 'safire' and w == 'passive' and ins == 'omplib':
                        #    fi_targets += ['fi-0' ,'fi-1-15' ]
                        for i in config.data[t][c]['inputs']:
                            for n in config.data[t][c][i][ins]['nthreads']:
                                print('\r', '<%8s> Processing %8s %8s %8s %8s %5s %3s %6s %6s' % ('FI', t, a, c, w, i, n, ins, 'fi'), end='')
                                #def parse_fi(resdir, tool, config, app, action, instrument, nthreads, inputsize, start, end, verbose):
                                #missing, timeout, crash, soc, benign, m_time, s_time, e_time = parse_fi(resdir, t, c, w, a, target, ins, n, i, start, end, False)
                                pending = parse_fi(resdir, t, c, w, a, 'fi', ins, n, i, start, end, False)
                                if pending:
                                    pending_apps.add( a )
                if pending_apps:
                    with open('todo-%s.sh'%( t ), 'a') as f:
                        runstr = '../sched.py -d $PROGDIR -r $RESDIR -a %s -t %s -x %s -s %s -e %s -c %s -l 1800 -N 4 -p batch -g' % (
                                ' '.join( pending_apps ), t, 'fi', start, end, c )
                        if w != '':
                            runstr += ' -w %s' % ( w )
                        runstr += '\n'
                        f.write( runstr )


def main():
    parser = argparse.ArgumentParser('Create todo scripts to run missing experiments')
    parser.add_argument('-r', '--resdir', help='results directory', required=True)
    parser.add_argument('-t', '--tools', help='tool to run', choices=['golden', 'safire', 'refine', 'pinfi', 'pinfi-detach' ], nargs='+', required=True)
    parser.add_argument('-ps', '--pstart', help='start trial', type=int, required=True)
    parser.add_argument('-pe', '--pend', help='end trial', type=int, required=True)
    parser.add_argument('-s', '--start', help='start trial', type=int, required=True)
    parser.add_argument('-e', '--end', help='end trial', type=int, required=True)
    parser.add_argument('-a', '--apps', help='applications to run ( ' + ' | '.join(data.apps) + ' | ALL ) ', nargs='+', required=True)
    args = parser.parse_args()

    if args.apps == ['ALL']:
        apps = data.apps
    else:
        apps = args.apps

    nested_dict = lambda: collections.defaultdict(nested_dict)
    results = nested_dict()

    #def analyze_profiles(resdir, tools, apps, config, espace):
    analyze_profiles(results, args.resdir, args.tools, apps, args.pstart, args.pend)
    #def analyze_fi(resdir, tools, apps, config, espace):
    analyze_fi(results, args.resdir, args.tools, apps, args.start, args.end)

if __name__ == "__main__":
    main()
