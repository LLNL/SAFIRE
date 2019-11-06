#!/usr/bin/env python3.6

import os
import sys
import subprocess
import timeit
import time
import multiprocessing as mp
import traceback
import argparse

import data

try:
    SLURM_PROCID = os.environ['SLURM_PROCID']
except:
    print('Env variable SLURM_PROCID is missing')
    sys.exit(1)

parser = argparse.ArgumentParser('Run a list of experiment tuples')
# tuple of 3: (trialdir, exelist str)
parser.add_argument('-e', '--env', help='environment variables to set', nargs=2, action='append')
parser.add_argument('-r', '--runlist', help='list to run', nargs=5, action='append', required=True)
args = parser.parse_args()

def run(e):
    trialdir= e[0]
    timeout = float( e[1] )
    # timeout 0 means no timeout
    if timeout == 0:
        timeout = None
    
    exelist = e[2].split()
    cleanstr = e[3]
    #print('trialdir:' + trialdir)
    #print('==== exelist ====')
    #print(exelist)
    #print('==== end of exelist ====')
    #print('==== cleanstr ====')
    #print(cleanstr)
    #print('==== end of cleanstr ====')

    out_file = open(trialdir + '/output.txt', 'w')
    err_file = open(trialdir + '/error.txt', 'w')
    ret = 0
    timed_out = False

    #print('==== before run ====')
    #print(exelist)
    #print('==== end before run ====')

    runenv = os.environ.copy()
    #newenv['LD_BIND_NOW'] = '1' # ggout ggin
    if args.env:
        for e in args.env:
            runenv[e[0]] = e[1]
            #print('setting %s=%s'%(e[0], e[1]) ) # ggout
    start = time.perf_counter()
    try:
        #exelist = ['env']
        p = subprocess.Popen(exelist, stdout=out_file, stderr=err_file, env=runenv, cwd=trialdir)
        p.wait(timeout)
        #p = subprocess.run(exelist, stdout=out_file, stderr=err_file, timeout=timeout)
        #p = subprocess.run(exelist, stdout=out_file, stderr=subprocess.DEVNULL, timeout=timeout)
        ret = p.returncode
    except subprocess.TimeoutExpired:
        timed_out = True
    xtime = time.perf_counter() - start

    out_file.close()
    err_file.close()

    # cleanup
    #print('Executing trialdir %s cleanstr: %s'%(trialdir, ', '.join(cleanstr) ) )
    p = subprocess.Popen(cleanstr, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, env=runenv, cwd=trialdir, shell=True)
    p.wait()

    #print('RET: ' + str(ret))
    ret_file = open(trialdir + '/ret.txt', 'w')
    if timed_out:
        ret_file.write('timeout\n')
        #print('Process timed out!')
    elif ret < 0:
        ret_file.write('crash, ' + str(ret) + '\n')
    elif ret > 0:
        ret_file.write('error, ' + str(ret) + '\n')
    else:
        ret_file.write('exit, ' + str(ret) + '\n')
    ret_file.close()

    #print('time: %.2f'%(xtime))
    with open(trialdir + '/time.txt', 'w') as f:
        f.write('%.2f'%(xtime) + '\n')

    #print('Profiling ' + ' '.join(e) + ' done')


#pool.map(run, args.runlist)#, chunksize=int( max( 1, len(args.runlist)/nworkers ) ) )
for r in [rr for rr in args.runlist if rr[0] == SLURM_PROCID]:
    run(r[1:])
