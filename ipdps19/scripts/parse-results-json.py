#!/usr/bin/env python3

import argparse
import re
import os
import collections
import json
import numpy as np
import scipy.stats as stats

import data
import fi_tools
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
    #print( profiledir ) # ggout

    for trial in range(start, end+1):
        trialdir = '%s/%s/'%(profiledir, trial)
        # Parse inscount for tools other than golden
        if do_inscount:
            fname = trialdir + fi_tools.files[tool]['inscount']
            if not os.path.isfile( fname ):
                continue # Skip # ggout
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
        try:
            with open(fname, 'r') as f:
                xtime.append( float(f.read()) )
        except FileNotFoundError:
            continue

    # Stats settings, CI and t-distro
    ci = 0.95
    t_critical = stats.t.ppf( q = (1 + ci) /2., df = end-start )
    #t_critical = 2.00 # ggout

    if do_inscount:
        #m_inscount = s_inscount = e_inscount = 0 # ggout
        #print( inscount ) # ggout
        m_inscount = int(np.mean(inscount))
        s_inscount = int( np.std( inscount, ddof=1 ) )
        #print( stats.sem(inscount ) ) # ggout
        #print( t_critical ) # ggout
        e_inscount = int(stats.sem(inscount) * t_critical)
        #stats.t.interval(0.95, end-start, loc=m_inscount, scale=stats.sem(inscount))
        stats_thread_inscount = []
        if config == 'omp':
            for i in range( 0, int(nthreads) ):
                #m_thread_inscount = s_thread_inscount = e_thread_inscount = 0 # ggout
                m_thread_inscount = int( np.mean( thread_inscount[i] ) )
                s_thread_inscount = int( np.std( thread_inscount[i], ddof=1 ) )
                e_thread_inscount = int( stats.sem( thread_inscount[i] ) * t_critical )
                stats_thread_inscount.append( [ m_thread_inscount, s_thread_inscount, e_thread_inscount ] )
    m_time = round( np.mean( xtime ), 2)
    s_time = round( np.std( xtime, ddof=1 ) )
    e_time = round( stats.sem( xtime ) * t_critical, 2 )

    if do_inscount:
        return m_time, s_time, e_time, m_inscount, s_inscount, e_inscount, stats_thread_inscount
    else:
        return m_time, s_time, e_time

def analyze_profiles(results, resdir, tools, apps, start, end):
    # golden baseline
    for c in config.data[t]['exec']:
        for w in config.data[t][c]['wait']:
            for a in apps:
                for i in config.data[t][c]['inputs']:
                    for n in espace[c][i]['nthreads']:
                        # XXX: golden has no instrumentation
                        print('\r', '<%8s> Processing %8s %8s %8s %8s %5s %3s %6s' % ('profile','golden', a, c, w, i, n, ''), end='' )
                        m_time, s_time, e_time = parse_profiles(resdir, 'golden', c, w, a, '', n, i, start, end)
                        results['profile']['golden'][w][a][c][i][n]['time']['mean'] = m_time
                        results['profile']['golden'][w][a][c][i][n]['time']['std'] = s_time
                        results['profile']['golden'][w][a][c][i][n]['time']['sem'] = e_time

    # fi tools
    for t in tools:
        for a in apps:
            for c in config:
                for w in espace[c]['wait']:
                    for i in espace[c]['inputs']:
                        for ins in espace[c]['instrument']:
                            for n in espace[c][i]['nthreads']:
                                print('\r', '<%8s> Processing %8s %8s %8s %8s %5s %3s %6s' % ('profile','golden', a, c, w, i, n, ''), end='' )
                                profiledir = '%s/%s/%s/%s/%s/%s/%s/%s/%s'%(resdir, t, w, c, a, 'profile', ins, n, i)
                                m_time, s_time, e_time, m_inscount, s_inscount, e_inscount, stats_thread_inscount = (
                                    parse_profiles(resdir, t, c, w, a, ins, n, i, start, end) )
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
        trialdir = '%s/%s/'%( basedir, trial )
        if not os.path.isfile(trialdir + '/ret.txt'):
            print('PENDING trial %d'%( trial ) )
            continue

        with open(trialdir + '/time.txt', 'r') as f:
            xtime.append( float( f.read() ) )

        with open(trialdir + 'ret.txt', 'r') as f:
            if not os.path.isfile(trialdir + fi_tools.files[tool]['injection']):
                missing += 1
            else:
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

    print('\tmissing: ' + str(missing) + ', timeout: ' + str(timeout) + ', crash: ' + str(crash) + ', soc: ' + str(soc) + ', benign: ' + str(benign))
    total = end - start + 1
    print('\t(%%) timeout %3.2f, crash %3.2f, soc %3.2f, benign %3.2f'%(timeout*100.0/total, crash*100.0/total, soc*100.0/total, benign*100.0/total) )
    print('===== END APP ' + app +' =====\n');

    m_time = np.mean( xtime )
    s_time = np.std( xtime, ddof = 1 )
    e_time = stats.sem( xtime )

    return missing, timeout, crash, soc, benign, m_time, s_time, e_time


def analyze_fi(results, resdir, tools, apps, config, espace, start, end):
# TODO: fix this to call parse_fi an fill the results nested dict
    for t in tools:
        for a in apps:
            for c in config:
                for w in espace[c]['wait']:
                    for i in espace[c]['inputs']:
                        for ins in espace[c]['instrument']:
                            for n in espace[c][i]['nthreads']:
                                fi_targets = ['fi']
                                if ins == 'omplib' and t == 'safire':
                                    fi_targets += ['fi-0' ,'fi-1-15' ]
                                for target in fi_targets:
                                    print('\r', '<%8s> Processing %8s %8s %8s %8s %5s %3s %6s %6s' % ('fi', t, a, c, w, i, n, ins, target), end='')
                                    #def parse_fi(resdir, tool, config, app, action, instrument, nthreads, inputsize, start, end, verbose):
                                    missing, timeout, crash, soc, benign, m_time, s_time, e_time = parse_fi(resdir, t, c, w, a, target, ins, n, i, start, end, False)
                                    results[target][t][w][a][c][i][n][ins]['outcomes']['missing'] = missing
                                    results[target][t][w][a][c][i][n][ins]['outcomes']['timeout'] = timeout
                                    results[target][t][w][a][c][i][n][ins]['outcomes']['crash'] = crash
                                    results[target][t][w][a][c][i][n][ins]['outcomes']['soc'] = soc
                                    results[target][t][w][a][c][i][n][ins]['outcomes']['benign'] = benign

                                    results[target][t][w][a][c][i][n][ins]['time']['mean'] = m_time
                                    results[target][t][w][a][c][i][n][ins]['time']['std'] = s_time
                                    results[target][t][w][a][c][i][n][ins]['time']['sem'] = e_time

def main():
    parser = argparse.ArgumentParser('Create json file of results')
    parser.add_argument('-r', '--resdir', help='results directory', required=True)
    parser.add_argument('-t', '--tools', help='tool to run', choices=['safire', 'refine', 'pinfi', 'pinfi-detach' ], nargs='+', required=True)
    parser.add_argument('-ps', '--pstart', help='start trial', type=int, required=True)
    parser.add_argument('-pe', '--pend', help='end trial', type=int, required=True)
    parser.add_argument('-s', '--start', help='start trial', type=int, required=True)
    parser.add_argument('-e', '--end', help='end trial', type=int, required=True)
    args = parser.parse_args()

    apps = data.apps

    nested_dict = lambda: collections.defaultdict(nested_dict)
    results = nested_dict()

    #def analyze_profiles(resdir, tools, apps, config, espace):
    analyze_profiles(results, args.resdir, args.tools, apps, args.pstart, args.pend)
    #def analyze_fi(resdir, tools, apps, config, espace):
    analyze_fi(results, args.resdir, args.tools, apps, args.start, args.end)

    json.dump(results, open('results.json', 'w'), sort_keys=True, indent=4 )

if __name__ == "__main__":
    main()
