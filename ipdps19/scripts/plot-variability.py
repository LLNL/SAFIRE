#!/usr/bin/env python3.6

import matplotlib.pyplot as plt
import matplotlib
import os
import sys
import argparse
import re
import numpy as np
import scipy.stats as stats
import json

import data
import fi_tools

def progress(total, completed):
    frac = completed/total
    full_progbar = 50
    filled_progbar = round(frac * full_progbar)

    print('\r','Status %5d / %5d'%(completed, total)
        , '#'*filled_progbar + '-'*(full_progbar - filled_progbar)
        , '[{:>7.2%}]'.format(frac)
        , end='' )

    sys.stdout.flush()

def autolabel(ax, bars, err):
    for i, bar in enumerate(bars):
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., 1.00*height + err[i], '%.1f%%'%( (err[i]*100.0)/height ), ha='center', va='bottom')

def main():
    parser = argparse.ArgumentParser('Plot variability')
    parser.add_argument('-r', '--resfile', help='results file', required=True)
    parser.add_argument('-t', '--tools', help='tool to run', choices=['refine', 'pinfi', 'refine-noff'], nargs='+', required=True)
    parser.add_argument('-s', '--start', help='start trial', type=int, required=True)
    parser.add_argument('-e', '--end', help='end trial', type=int, required=True)
    args = parser.parse_args()

    if not os.path.isfile(args.resfile):
        parser.error('results file does not exist')
    if args.start > args.end:
        parser.error('start must be <= end')

    # load results
    results = json.load( open( args.resfile, 'r' ) )

    config = [ 'serial', 'omp' ]
    #config = [ 'omp' ] # ggout
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
    #apps = [ 'AMG' ]
    apps = data.apps

    ispace = [ (a,c,i,n,ins)
                for a in apps
                for c in config
                for i in espace[c]['inputs']
                for n in espace[c][i]['nthreads']
                for ins in espace[c]['instrument']
            ]

    total = len( ispace )
    completed = 0;
    matplotlib.rcParams.update({'font.size': 16})
    for a in apps:
        for c in config:
            for i in espace[c]['inputs']:
                for n in espace[c][i]['nthreads']:
                    for ins in espace[c]['instrument']:
                        y = []
                        yerr = []
                        for t in args.tools:
                            m_inscount = results['profile'][t][a][c][i][n][ins]['inscount']['mean']
                            e_inscount = results['profile'][t][a][c][i][n][ins]['inscount']['sem']
                            y.append( m_inscount )
                            yerr.append( e_inscount )

                        #print(m_inscount_list)
                        #print(err_inscount_list)
                        x = range( 0, len( args.tools ) )

                        plt.title('%s %s %s %s %s'%(a, i, c, n, ins) )
                        plt.tick_params(
                                axis='x',          # changes apply to the x-axis
                                which='both',      # both major and minor ticks are affected
                                bottom='off',      # ticks along the bottom edge are off
                                top='off',         # ticks along the top edge are off
                                labelbottom='on') # labels along the bottom edge are off
                        plt.gca().spines['top'].set_visible( False )
                        plt.gca().spines['right'].set_visible( False )
                        plt.ylabel( 'Instructions' )
                        bars = plt.bar( x, y, yerr=yerr, tick_label=[x.upper() for x in args.tools], capsize=8)
                        autolabel(plt.gca(), bars, yerr)
                        plt.tight_layout()
                        plt.ticklabel_format(style='scientific', axis='y', scilimits=(0,0))
                        plt.savefig('%s-%s-%s-%s-%s.pdf'%(a, i, c, n, ins) )
                        plt.close()
                        completed += 1
                        progress(total, completed)

if __name__ == "__main__":
    main()
