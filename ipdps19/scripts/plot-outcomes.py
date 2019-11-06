#!/usr/bin/env python3

import os
import sys
import data
import matplotlib.pyplot as plt
import matplotlib
import argparse
import json

def autolabel(ax, bars, err):
    for rect in bars:
        height = rect.get_height()
        ax.text(rect.get_x() + rect.get_width()/2., 1.00*height + err, '%.1f'%(height), ha='center', va='bottom')

def main():
    parser = argparse.ArgumentParser('This script creates outcome plots')
    parser.add_argument('-r', '--resfile', help='results file', required=True)
    parser.add_argument('-c', '--comparators', help='tool to compare against baseline', nargs=6, action='append', required=True )
    parser.add_argument('-a', '--apps', help='applications to run', choices=data.apps+['ALL'], nargs='+', required=True)
    parser.add_argument('-s', '--start', help='start trial', type=int, required=True)
    parser.add_argument('-e', '--end', help='end trial', type=int, required=True)
    #parser.add_argument('-o', '--output', help='output filename', required=True )
    args = parser.parse_args()

    if not os.path.isfile( args.resfile ):
        parser_error('results file does not exists')
    if args.start > args.end:
        parser_error('start must be < end')

    if args.apps == ['ALL']:
        apps = data.apps
    else:
        apps = args.apps

    results = json.load( open( args.resfile, 'r' ) )

    width = 0.75
    scale = 1
    print('Creating plots...')

    matplotlib.rcParams.update({'font.size': 16})
    x = range(0, len(args.comparators) * scale, scale)
    outcomes = ['missing', 'timeout', 'crash', 'soc', 'benign' ]

    for a in apps:
        for o in outcomes:
            y = []
            for c in args.comparators:
                outcomes = results['fi'][ c[0] ][ c[1] ][ a ][ c[2] ][ c[3] ][ c[4] ][ c[5] ]['outcomes']
                y.append( outcomes[ o ] )
                #y = [yi*100.0/(args.end-args.start+1) for yi in y]


            plt.title('%s %s'%( a, o ) )
            bars = plt.bar(x, y, width, yerr=[3]*len(y) )
            autolabel(plt.gca(), bars, 3)
            plt.gca().spines['top'].set_visible(False)
            plt.gca().spines['right'].set_visible(False)
            plt.tick_params(
                axis='x',          # changes apply to the x-axis
                which='both',      # both major and minor ticks are affected
                bottom='off',      # ticks along the bottom edge are off
                top='off',         # ticks along the top edge are off
                labelbottom='on') # labels along the bottom edge are off
            plt.xticks( x, [ i[0].upper() for i in args.comparators ] )
            plt.savefig( 'outcomes-%s-%s.pdf'%( a, o ) )
            plt.close()

#for a in args.apps:
    #    for c in config:
    #        for w in espace[c]['wait']:
    #            for i in espace[c]['inputs']:
    #                for n in espace[c][i]['nthreads']:
    #                    for ins in espace[c]['instrument']:
    #                        width = 0.5
    #                        x = range(0, len(args.tools) * scale, scale)
    #                        for t in args.tools:
    #                            outcomes = ['missing', 'timeout', 'crash', 'soc', 'benign' ]
    #                            y = []
    #                            missing = results['fi'][t][w][a][c][i][n][ins]['outcomes']['missing']
    #                            y.append( missing )
    #                            timeout = results['fi'][t][w][a][c][i][n][ins]['outcomes']['timeout']
    #                            y.append( timeout )
    #                            crash = results['fi'][t][w][a][c][i][n][ins]['outcomes']['crash']
    #                            y.append( crash )
    #                            soc = results['fi'][t][w][a][c][i][n][ins]['outcomes']['soc']
    #                            y.append( soc )
    #                            benign = results['fi'][t][w][a][c][i][n][ins]['outcomes']['benign']
    #                            y.append( benign )
    #                            all_out = missing + timeout + crash + soc + benign
    #                            y = [ yi * 100 / all_out for yi in y ]

    #                            matplotlib.rcParams.update({'font.size': 16})
    #                            plt.gca().spines['top'].set_visible(False)
    #                            plt.gca().spines['right'].set_visible(False)
    #                            plt.title('outcomes-%s-%s-%s-%s-%s-%s-%s.pdf'%( t, w, a, c, i, n, ins ) )
    #                            #plt.xticks( x, outcomes )
    #                            plt.tick_params(
    #                                axis='x',          # changes apply to the x-axis
    #                                which='both',      # both major and minor ticks are affected
    #                                bottom='off',      # ticks along the bottom edge are off
    #                                top='off',         # ticks along the top edge are off
    #                                labelbottom='off') # labels along the bottom edge are off

    #                            bottom = 0
    #                            for j, o in enumerate( outcomes ) :
    #                                bars = plt.bar(x, y[j], width, bottom=bottom, label=outcomes[j] )
    #                                autolabel(plt.gca(), bars, bottom)
    #                                bottom += y[j]
    #                        plt.legend()
    #                        plt.savefig( 'pct-%s-%s-%s-%s-%s-%s-%s.pdf'%( t, w, a, c, i, n, ins ) )
    #                        plt.close()

if __name__ == "__main__":
    main()

