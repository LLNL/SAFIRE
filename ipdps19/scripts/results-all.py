#!/usr/bin/env python3

import argparse
import subprocess
import data
import os
import re
import analysis

def main():
    parser = argparse.ArgumentParser('Run profiling experiments')
    parser.add_argument('-r', '--resdir', help='results directory', required=True)
    parser.add_argument('-a', '--apps', help='applications to run', choices=data.apps+['ALL'], nargs='+', required=True)
    parser.add_argument('-t', '--tools', help='tool to run', choices=['safire', 'refine', 'pinfi', 'pinfi-detach' ], nargs='+', required=True)
    parser.add_argument('-x', '--action', help='results for action', choices=['profile', 'fi'], required=True)
    parser.add_argument('-s', '--start', help='start trial', type=int, required=True)
    parser.add_argument('-e', '--end', help='end trial', type=int, required=True)
    parser.add_argument('-w', '--wait', help='wait policy', choices=['passive', 'active', ''], required=True)
    args = parser.parse_args()

    # Error checking
    assert os.path.isdir(args.resdir), 'Applications directory: ' + args.resdir + 'does not exist'
    assert args.start <= args.end, 'Start must be < end'
    if args.apps == ['ALL']:
        args.apps = data.apps

    config = [ 'serial', 'omp' ]
    espace = {
            'serial': {
                'inputs':[ 'small' ],
                'instrument': [''],
                'small' : { 'nthreads' : [''] }
                },
            'omp': {
                'inputs':[ 'small', 'large' ],
                'instrument': ['all', 'app', 'omplib'],
                'small' : { 'nthreads' : [ '16', '8', '1' ] },
                'large' : { 'nthreads' : [ '16', '8' ] },
                }
            }

    #def check(tool, config, apps, action, instrument, nthreads, inputsize, start, end)
    for t in args.tools:
        for a in args.apps:
            for c in config:
                for i in espace[c]['inputs']:
                    for n in espace[c][i]['nthreads']:
                        for ins in espace[c]['instrument']:
                            if c == 'serial':
                                fname ='results-%s-%s-serial-%s-%s-%s.txt'%( a, t, i, args.start, args.end)
                            else: #omp
                                fname = 'results-%s-%s-omp-%s-%s-%s-%s-%s.txt'%( a, t, i, n, ins, args.start, args.end)

                            if not os.path.isfile(fname):
                                #def result(resdir, tool, config, app, action, instrument, nthreads, inputsize, start, end, verbose)
                                analysis.results(args.resdir, t, c, args.wait, a, args.action, ins, n, i, args.start, args.end, False)

if __name__ == "__main__":
    main()
