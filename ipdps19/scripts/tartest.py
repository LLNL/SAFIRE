#!/usr/bin/env python3

import tarfile
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

# Parse profiling files
def parse_profiles(resfile, tool, config, wait, app, instrument, nthreads, inputsize, start, end):
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

    profiledir = '%s/%s/%s/%s/%s/%s/%s/%s'%(tool, config, wait, app, 'profile', instrument, nthreads, inputsize)
    tar = tarfile.open(resfile)
    #print( profiledir ) # ggout

    for trial in range(start, end+1):
        trialdir = '%s/%s/'%(profiledir, trial)

        # Check inscount
        if do_inscount:
            fname = trialdir + fi_tools.files[tool]['inscount']
            try:
                #if not os.path.isfile( fname ):
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
            except FileNotFoundError:
                print('Missing %s'%( fname ) )
                #continue
                sys.exit(123)

        # read execution time
        fname = trialdir + 'time.txt'
        fname = 'golden/serial/AMG/profile/small/1/time.txt'
        try:
            #with open(fname, 'r') as f:
            with tar.extractfile(fname) as f:
                xtime.append( float(f.read()) )
        except FileNotFoundError:
            print('Missing %s'%( fname ) )

    print( xtime )
    sys.exit(123)
    # Stats settings, CI and t-distro
    ci = 0.95
    t_critical = stats.t.ppf( q = (1 + ci) /2., df = end-start )
    #t_critical = 2.00 # ggout

    if do_inscount:
        #m_inscount = s_inscount = e_inscount = 0 # ggout
        #print( inscount ) # ggout
        m_inscount = int( np.mean( inscount ) )
        # TODO: make sure there are more than 1 measurement for all tools
        if len( inscount ) > 1:
            s_inscount = int( np.std( inscount, ddof=1 ) )
            e_inscount = int( stats.sem( inscount ) * t_critical)
            #print( stats.sem(inscount ) ) # ggout
            #print( t_critical ) # ggout
            #stats.t.interval(0.95, end-start, loc=m_inscount, scale=stats.sem(inscount))
        else:
            s_inscount = int( np.std( inscount ) )
            e_inscount = 0
        stats_thread_inscount = []
        if config == 'omp':
            for i in range( 0, int(nthreads) ):
                #m_thread_inscount = s_thread_inscount = e_thread_inscount = 0 # ggout
                m_thread_inscount = int( np.mean( thread_inscount[i] ) )
                # TODO: make sure there are more than 1 measurement for all tools
                if len( thread_inscount[i] ) > 1: 
                    s_thread_inscount = int( np.std( thread_inscount[i], ddof=1 ) )
                    e_thread_inscount = int( stats.sem( thread_inscount[i] ) * t_critical )
                else:
                    s_thread_inscount = 0
                    e_thread_inscount = 0

                stats_thread_inscount.append( [ m_thread_inscount, s_thread_inscount, e_thread_inscount ] )

    m_time = round( np.mean( xtime ), 2)
    s_time = round( np.std( xtime, ddof=1 ) )
    e_time = round( stats.sem( xtime ) * t_critical, 2 )

    if do_inscount:
        return m_time, s_time, e_time, m_inscount, s_inscount, e_inscount, stats_thread_inscount
    else:
        return m_time, s_time, e_time, 0, 0, 0, []

# Parse fault injection files
def parse_fi(resdir, tool, config, wait, app, action, instrument, nthreads, inputsize, start, end, verbose):
    print('===== APP %s %s %s %s %s %s %s %s %s  ====='%(app, tool, config, action, instrument, nthreads, inputsize, start, end) )
    missing = timeout = crash = soc = benign = 0
    # base app dir
    rootdir = '%s/%s/%s/%s/'%(resdir, 'golden', config, wait)
    # specific trial dir
    profiledir = '%s/%s/profile/%s/%s/'%(rootdir, app, nthreads, inputsize )

    # Use the first experiment on golden to get the valid output
    # TODO: check whether outputs with omp may differ but still valid
    trialdir = profiledir + '1/'

    verify_list = []
    with open(trialdir + 'output.txt', 'r') as f:
        for v in data.programs[config][app]['verify'][inputsize]:
            #print(v)
            m = re.findall(v, f.read())
            verify_list += m
    #print(verify_list)
    assert verify_list, 'verify_list cannot be empty file: ' + trialdir + 'output.txt' + ', verify:' + ' '.join(data.programs[config][app]['verify'][inputsize])


    basedir = '%s/%s/%s/%s/%s/%s/%s/%s/%s/'%(resdir, tool, config, wait, app, action, instrument, nthreads, inputsize)

    xtime = []
    for trial in range(start, end+1):
        print('\r','processing %d' % ( trial ), end='' ) # ggout
        trialdir = '%s/%s/'%( basedir, trial )
        try:
            with open(trialdir + '/ret.txt', 'r'):
                pass
        except FileNotFoundError:
            print('PENDING trialdir %s'%( trialdir ) )
            #continue
            sys.exit(123)
        
        with open(trialdir + '/time.txt', 'r') as f:
            xtime.append( float( f.read() ) )

        with open(trialdir + '/ret.txt', 'r') as f:
            #if not os.path.isfile(trialdir + fi_tools.files[tool]['injection']):
            try:
                with open(trialdir + fi_tools.files[tool]['injection'], 'r'):
                    res = f.read()
                    res = res.strip().split(',')
                    if res[0] == 'timeout':
                        timeout += 1
                    elif res[0] == 'crash':
                        crash += 1
                    elif res[0] == 'error':
                        crash += 1
                    elif res[0] == 'exit':
                        with open(trialdir + '/' + 'output.txt', 'r') as f:
                            #print('open: ' + trialdir +'/' + 'output.txt')
                            verify_out = []
                            for v in data.programs[config][app]['verify'][inputsize]:
                                m = re.findall(v, f.read())
                                verify_out += m
                            verified = True
                            for out in verify_list:
                                if not out in verify_out:
                                    verified = False
                                    break
                            if verbose:
                                print('*** verify ***')
                                print(verify_list)
                                print(verify_out)
                                print('trial %d verified: %s'%(trial, verified))
                                print('*** end verify ***')
                            if verified:
                                benign += 1
                            else:
                                soc += 1
                                #TODO: extend verifier with tolerance
                            #print('\ttimeout: ' + str(timeout) + ', crash: ' + str(crash) + ', soc: ' + str(soc) + ', benign: ' + str(benign))
                    else:
                        print('Invalid result ' + tool + ' ' + app + ' ' + str(trial) +' :' + str(res[0]))

            except FileNotFoundError:
                missing += 1
                

    print('\tmissing: ' + str(missing) + ', timeout: ' + str(timeout) + ', crash: ' + str(crash) + ', soc: ' + str(soc) + ', benign: ' + str(benign))
    total = end - start + 1
    #print('\t(%%) timeout %3.2f, crash %3.2f, soc %3.2f, benign %3.2f'%(timeout*100.0/total, crash*100.0/total, soc*100.0/total, benign*100.0/total) )
    print('===== END APP ' + app +' =====\n');

    m_time = np.mean( xtime )
    s_time = np.std( xtime, ddof = 1 )
    e_time = stats.sem( xtime )

    return missing, timeout, crash, soc, benign, m_time, s_time, e_time

def analyze(results, resfile, tools, apps, pstart, pend, start, end):
    for t in tools:
        for c in config.data[t]['exec']:
            for w in config.data[t][c]['wait']:
                for a in apps:
                    for ins in config.data[t][c]['instrument']:
                        # XXX: REMOVE side-effect of active missing, think about moving to config data if used
                        #fi_targets = ['fi']
                        #if t == 'safire' and w == 'passive' and ins == 'omplib':
                        #    fi_targets += ['fi-0' ,'fi-1-15' ]
                        for i in config.data[t][c]['inputs']:
                            for n in config.data[t][c][i][ins]['nthreads']:
                                #print('\r', '<%8s> Processing %8s %10s %8s %8s %5s %3s %6s %6s' % ('Profile', t, a, c, w, i, n, ins, 'profile'), end='')
                                print('<%8s> Processing %8s %10s %8s %8s %5s %3s %6s %6s' % ('Profile', t, a, c, w, i, n, ins, 'profile') )
                                #profiledir = '%s/%s/%s/%s/%s/%s/%s/%s/%s'%(resdir, t, w, c, a, 'profile', ins, n, i)
                                m_time, s_time, e_time, m_inscount, s_inscount, e_inscount, stats_thread_inscount = (
                                        parse_profiles(resfile, t, c, w, a, ins, n, i, pstart, pend) )

                                results['profile'][t][w][a][c][i][n][ins]['time']['mean'] = m_time
                                results['profile'][t][w][a][c][i][n][ins]['time']['std'] = s_time
                                results['profile'][t][w][a][c][i][n][ins]['time']['sem'] = e_time

                                results['profile'][t][w][a][c][i][n][ins]['inscount']['mean'] = m_inscount
                                results['profile'][t][w][a][c][i][n][ins]['inscount']['std'] = s_inscount
                                results['profile'][t][w][a][c][i][n][ins]['inscount']['sem'] = e_inscount
                                if c == 'omp':
                                    for j, [m, s, e] in enumerate( stats_thread_inscount ):
                                       results['profile'][t][w][a][c][i][n][ins]['thread_inscount'][j]['mean'] = m
                                       results['profile'][t][w][a][c][i][n][ins]['thread_inscount'][j]['std'] = s
                                       results['profile'][t][w][a][c][i][n][ins]['thread_inscount'][j]['sem'] = e

                                # XXX: skip fi analysis for golden, TODO: move that to config file
                                if t == 'golden':
                                    continue

                                print('\r', '<%8s> Processing %8s %8s %8s %8s %5s %3s %6s %6s' % ('FI', t, a, c, w, i, n, ins, 'fi'), end='')
                                #def parse_fi(resdir, tool, config, app, action, instrument, nthreads, inputsize, start, end, verbose):
                                missing, timeout, crash, soc, benign, m_time, s_time, e_time = ( 
                                        parse_fi(resdir, t, c, w, a, 'fi', ins, n, i, start, end, False) )

                                results['fi'][t][w][a][c][i][n][ins]['outcomes']['missing'] = missing
                                results['fi'][t][w][a][c][i][n][ins]['outcomes']['timeout'] = timeout
                                results['fi'][t][w][a][c][i][n][ins]['outcomes']['crash'] = crash
                                results['fi'][t][w][a][c][i][n][ins]['outcomes']['soc'] = soc
                                results['fi'][t][w][a][c][i][n][ins]['outcomes']['benign'] = benign

                                results['fi'][t][w][a][c][i][n][ins]['time']['mean'] = m_time
                                results['fi'][t][w][a][c][i][n][ins]['time']['std'] = s_time
                                results['fi'][t][w][a][c][i][n][ins]['time']['sem'] = e_time

def main():
    parser = argparse.ArgumentParser('Create json file of results')
    parser.add_argument('-r', '--resfile', help='results file', required=True)
    parser.add_argument('-o', '--output', help='output file', required=True)
    parser.add_argument('-t', '--tools', help='tool to run', choices=['golden', 'safire', 'refine', 'pinfi', 'pinfi-detach' ], nargs='+', required=True)
    parser.add_argument('-ps', '--pstart', help='start trial', type=int, required=True)
    parser.add_argument('-pe', '--pend', help='end trial', type=int, required=True)
    parser.add_argument('-s', '--start', help='start trial', type=int, required=True)
    parser.add_argument('-e', '--end', help='end trial', type=int, required=True)
    args = parser.parse_args()

    apps = data.apps

    nested_dict = lambda: collections.defaultdict(nested_dict)
    results = nested_dict()

    analyze(results, args.resfile, args.tools, apps, args.pstart, args.pend, args.start, args.end)

    json.dump(results, open(args.output, 'w'), sort_keys=True, indent=4 )

if __name__ == "__main__":
    main()
