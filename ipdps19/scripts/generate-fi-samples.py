#!/usr/bin/env python3.6

import os
import sys
import argparse
import re
import numpy as np
import random
from collections import Counter
from bashplotlib.histogram import plot_hist 
import itertools


import data
import fi_tools

# Parse profiling data to get stats
def parse_profile(basedir, tool, config, nthreads, input_size, start, end):
    inscount = []
    if config == 'omp':
        thread_inscount = {}
        for i in range(0, int(nthreads) ):
            thread_inscount[i] = []
    xtime = []

    for trial in range(start, end+1):
        trialdir = '%s/%s/'%(basedir, trial)
        fname = trialdir + fi_tools.files[tool]['inscount']
        # read instruction count
        #print('fname %s'%(fname))
        with open(fname, 'r') as f:
            fdata = f.read()
            #print(data)
            if config == 'omp':
                m = re.findall('thread=(\d+), fi_index=(\d+)', fdata)
                for i in m:
                    thread_inscount[ int(i[0]) ].append( int(i[1]) )

            # XXX: ^ and re.MULTILINE to handle omp instcount (serial works too)
            m = re.search( '^fi_index=(\d+)', fdata, re.MULTILINE )
            inscount.append( int(m[1]) )
        
        # read execution time
        fname = trialdir + 'time.txt'
        with open(fname, 'r') as f:
            xtime.append( float(f.read()) )

    m_inscount = int(np.mean(inscount))
    s_inscount = np.std(inscount)
    m_thread_inscount = []
    if config == 'omp':
        #print( thread_inscount ) # ggout
        for i in range( 0, int(nthreads) ):
            m_thread_inscount.append( (i, int(np.mean( thread_inscount[i] ) )) )
    m_time = np.mean(xtime)

    # create mean_time.txt for setting the timeout
    fname = '%s/mean_time.txt'%(basedir)
    #try:
    #    with open( fname, 'r') as f:
    #        m_time_old = float( f.read() )
    #        print('mean time read %.2f new %.2f' % ( m_time_old, m_time ) )
    #except FileNotFoundError:
    #    pass
    #print(fname)
    with open(fname, 'w') as f:
        f.write( '%.2f\n'%(m_time) )

    return m_time, m_inscount, s_inscount, m_thread_inscount

def write_fi_files(basedir, tool, config, samples, m_thread_inscount):
    # XXX: used for omp plotting check
    fi_threads = []
    nthreads = len(m_thread_inscount)

    for trial, target in enumerate(samples, 1):
        #print('target %d'%(target)) # ggout
        trialdir = '/%s/%s/'%(basedir, trial)
        #print(trialdir)
        fname = trialdir + fi_tools.files[tool]['target']
        if not os.path.exists(trialdir):
            os.makedirs(trialdir)
        #print('fname %s'%(fname))
        if not os.path.isfile(fname):
            # XXX: need to assign thread and fi_index
            # order instructions from 0...n thread
            print('GENERATED file')
            if config == 'omp':
                sum_inscount = 0
                # iterate over threads
                for thread, inscount in m_thread_inscount:
                    if target <= ( sum_inscount + inscount ):
                        fi_index = target - sum_inscount
                        fi_threads.append(thread)
                        break

                    sum_inscount += inscount

                #print('target=%d'%(target) )
                #print('thread=%d, fi_index=%d\n'%( thread, fi_index ))
                #input('press key to continue...')
                with open(fname, 'w') as f:
                    f.write('thread=%d, fi_index=%d\n'%( thread, fi_index ))
            else:
                with open(fname, 'w') as f:
                    f.write( 'fi_index=%d\n'%(target) )
    
    #if fi_threads:
    #    print(Counter(fi_threads).keys())
    #    print(Counter(fi_threads).values())
    #    print(nthreads)
    #    plot_hist(fi_threads, title='Thread distro', bincount=100, xlab=True)

def main():
    parser = argparse.ArgumentParser('Generate FI samples')
    parser.add_argument('-r', '--resdir', help='results directory', required=True)
    parser.add_argument('-o', '--outdir', help='output directory', required=True)
    parser.add_argument('-t', '--tool', help='tool to run', choices=['safire', 'refine', 'pinfi', 'pinfi-detach' ], required=True)
    parser.add_argument('-a', '--apps', help='applications to run ( ' + ' | '.join(data.apps) + ' | ALL ) ', nargs='+', required=True)
    parser.add_argument('-c', '--config', help='run configuration \n <serial | omp> <nthreads> <all | app | omplib>)', nargs='+', required=True)
    parser.add_argument('-i', '--input', help='input size', choices=['test', 'small', 'large'], required=True)
    parser.add_argument('-ps', '--pstart', help='profiling start', type=int, required=True)
    parser.add_argument('-pe', '--pend', help='profilng end', type=int, required=True)
    parser.add_argument('-g', '--generate', help='generate FI samples', action='store_true')
    parser.add_argument('-n', '--nsamples', help='number of FI samples', type=int, required=True)
    parser.add_argument('-tt', '--targetthreads', help='threads to target', type=int, nargs='+')
    parser.add_argument('-w', '--wait', help='wait policy', choices=['passive', 'active', ''], required=True)
    args = parser.parse_args()

    # Error checking
    assert os.path.isdir(args.resdir), 'Results directory: ' + args.resdir + 'does not exist'
    if args.apps == ['ALL']:
        args.apps = data.apps
    else:
        for a in args.apps:
            assert a in data.apps, 'Application: ' + a + ' is invalid'
    assert args.pstart <= args.pend, 'Start must be < end'
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

    if args.targetthreads and not config == 'omp':
        parser_error('targetthreads can be used only in omp configuration')
    assert args.nsamples > 0, 'Number of FI samples must be > 0'

    # Generate random samples and fi files
    for app in args.apps:
        print('==== ' + app + ' ====')
        profiledir = '%s/%s/%s/%s/%s/%s/%s/%s/%s'%(args.resdir, args.tool, config, args.wait, app, 'profile', instrument, nthreads, args.input)

        m_time, m_inscount, s_inscount, m_thread_inscount = parse_profile(profiledir, args.tool, config, nthreads, args.input, args.pstart, args.pend)
        if m_thread_inscount:
            print('mean inst. per thread')
            print(m_thread_inscount)
        print('mean inscount %d std %.2f%%'%(m_inscount, s_inscount*100.0/m_inscount) )
        print('mean time %.2f'%(m_time) )

        if args.generate :
            # create fi samples
            if args.targetthreads:
                print(args.targetthreads)
                all_sum_inscount = 0
                sum_inscount = 0
                mapping = []
                # iterate threads and linearize over selected ones
                for thread, inscount in m_thread_inscount:
                    if thread in args.targetthreads:
                        mapping.append( (sum_inscount+1, sum_inscount + inscount, all_sum_inscount - sum_inscount) )
                        sum_inscount += inscount
                    all_sum_inscount += inscount

                samples = random.sample( range(1, sum_inscount+1), args.nsamples)
                # re-map samples to original order
                for i, s in enumerate( samples ):
                    for first, last, offset in mapping:
                        if first <= s and s <= last:
                            samples[i] = s + offset
                            break
            else:
                samples = random.sample(range(1, m_inscount+1), args.nsamples)

            fidir = '%s/%s/%s/%s/%s/%s/%s/%s/%s'%(args.resdir, args.tool, config, args.wait, app, args.outdir, instrument, nthreads, args.input)
            
            write_fi_files(fidir, args.tool, config, samples, m_thread_inscount)

            samples = [n/m_inscount for n in samples]
            #plot_hist(samples, title='Target distro', bincount=100, xlab=True)
        print('==== END ' + app + ' ====')

if __name__ == "__main__":
    main()
