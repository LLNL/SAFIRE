#!/usr/bin/env python3

import sys
import argparse
import re
import os
import collections
import json
import numpy as np
import scipy.stats as stats

import data
import fi_tools
import config

def main():
    parser = argparse.ArgumentParser('Create json file of filesizes')
    parser.add_argument('-p', '--progdir', help='program directory', required=True)
    parser.add_argument('-o', '--output', help='output file', required=True)
    args = parser.parse_args()

    apps = data.apps

    nested_dict = lambda: collections.defaultdict(nested_dict)
    results = nested_dict()

    # Get filesize of binaries in serial execution
    for t in [ 'golden', 'refine', 'safire' ]:
        for a in apps:
            #print( a )
            progbin = args.progdir + '/' + t + '/serial/' + data.programs['serial'][a]['exec']['small'][0]
            size = os.stat(progbin).st_size
            print( '%s %s' % ( progbin, size) )
            results['serial'][t][a]['size'] = size

    # Get filesize of binaries in openmp execution
    for t in [ 'golden', 'refine', 'safire' ]:
        for a in apps:
            for i in [ 'large' ]:
                #print( a )
                progbin = args.progdir + '/' + t + '/omp/' + data.programs['omp'][a]['exec'][i][0]
                size = os.stat(progbin).st_size 
                print( '%s %s' % ( progbin, size) )
                results['omp'][t][a]['size'] = size

    progbin = os.environ['HOME'] + '/usr/local/lib/libomp.so'
    size = os.stat(progbin).st_size
    print( '%s %s' % ( progbin, size ) )
    results['omp']['golden']['libomp']['size'] = size

    progbin = os.environ['HOME'] + '/usr/local/refine/lib/libomp.so'
    size = os.stat(progbin).st_size
    print( '%s %s' % ( progbin, size ) )
    results['omp']['refine']['libomp']['size'] = size


    progbin = os.environ['HOME'] + '/usr/local/safire/lib/libomp.so'
    size = os.stat(progbin).st_size
    print( '%s %s' % ( progbin, size ) )
    results['omp']['safire']['libomp']['size'] = size


    json.dump(results, open(args.output, 'w'), sort_keys=True, indent=4 )

if __name__ == "__main__":
    main()
