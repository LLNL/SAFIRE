#!/usr/bin/env python3.6

import data
import argparse
import os
import fi_tools
import sys
import itertools
import config

try:
    homedir = os.environ['HOME']
except:
    print('Env variable HOME is missing')
    sys.exit(1)

def check(appdir, resdir, tool, config, wait, apps, action, instrument, nthreads, inputsize, start, end, generate):
    exps=[]
    # Create fault injection experiment tuples
    for app in apps:
        basedir = '%s/%s/%s/%s/%s/%s/%s/%s/%s/'%(resdir, tool, config, wait, app, action, instrument, nthreads, inputsize)
        if action == 'fi' or action == 'fi-0' or action == 'fi-1-15':
            # get timeout
            profiledir = '%s/%s/%s/%s/%s/%s/%s/%s/%s'%(resdir, tool, config, wait, app, 'profile', instrument, nthreads, inputsize)
            fname = '/%s/mean_time.txt'%(profiledir)
            with open(fname, 'r') as f:
                proftime = float( f.read() )
            #print('Read mean profiling time: %.2f, setting timeout 10x:  %.2f'%(timeout, timeout*20) )
            timeout = round(3 * proftime, 2)
        else: # profile, look opportunistically for previous runs
            # get timeout
            profiledir = '%s/%s/%s/%s/%s/%s/%s/%s/%s/1/'%(resdir, tool, config, wait, app, 'profile', instrument, nthreads, inputsize)
            fname = '/%s/time.txt'%(profiledir)
            if os.path.isfile( fname ):
                with open(fname, 'r') as f:
                    proftime = float( f.read() )
                # XXX: PINFI instruction counts BB, but faultinjections is INS! Use a large timeout
                #print('Read mean profiling time: %.2f, setting timeout 10x:  %.2f'%(timeout, timeout*20) )
                timeout = round(3 * proftime, 2)
            else:
                timeout = 0

        for trial in range(start, end+1):
            trialdir = basedir + '/' + str(trial) +'/'
            #print(trialdir)
            # Skip already done experiments
            #print('CHECK to skip %s/ret.txt'%(trialdir) )
            if os.path.isfile(trialdir+'/ret.txt'):
                #print(trialdir)
                print('\r','Found %10s trial %4s'%( app, trial ), end="" )
                continue
            #print('Adding %s trial %s'%( app, trial ) )

            if action == 'profile':
                # create trialdir
                if generate:
                    if not os.path.exists(trialdir):
                        os.makedirs(trialdir)
            elif action == 'fi' or action == 'fi-0' or action == 'fi-1-15':
                assert os.path.exists(trialdir), 'Trialdir %s does not exist, forgot to run generate-fi-samples?'%(trialdir)
                assert os.path.isfile(trialdir + '/'+ fi_tools.files[tool]['target']),'Trialdir %s does not exist, forgot to run generate-fi-samples?'%(trialdir)
                #assert not os.path.isfile(trialdir + '/'+ fi_tools.files[args.tool]['injection']),\
                #'Reproducible injection is not supported: ' + trialdir + '/' + fi_tools.files[args.tool]['injection']
                #if os.path.isfile(trialdir + '/'+ fi_tools.files[tool]['injection']):
                    #print('WARNING: Reproducible injection is not supported, found injection file')
                    #os.remove(trialdir + '/'+ fi_tools.files[tool]['injection'])
            else:
                assert False, 'Invalid action:' + action

            # Create executable
            ## Find the program binary to run
            if tool == 'pinfi' or tool == 'golden' or tool == 'pinfi-detach':
                compiled = 'golden'
            elif tool == 'safire':
                if instrument == 'omplib':
                    compiled = 'golden'
                else:
                    compiled = 'safire'
            elif tool == 'refine':
                if instrument == 'omplib':
                    compiled = 'golden'
                else:
                    compiled = 'refine'
            else:
                print('Unknown tool' + tool)
                sys.exit(1)
            rootdir = '%s/%s/%s/'%(appdir, compiled, config)

            ## Build the exelist
            ### Any tool specific exelist header
            fi_exelist=[]
            if tool in fi_tools.exelist:
                if config == 'serial':
                    fi_exelist = fi_tools.exelist[tool][config][action]
                else:
                    fi_exelist = fi_tools.exelist[tool][config][action][instrument]
            #print(fi_exelist)

            ### Patch APPDIR and NTHREADS if needed
            # XXX: replace $APPDIR, needed only for CoMD
            exelist = [ s.replace( '$APPDIR', '%s/%s'%(rootdir, app) ) for s in data.programs[config][app]['exec'][inputsize] ]
            # XXX: replace NTHREADS, needed only for XSBench omp
            exelist = [ s.replace( '$NTHREADS', nthreads ) for s in exelist ]
            # XXX: note using rootdir, program binary in data is relative to that
            exelist = fi_exelist + [ rootdir + exelist[0] ] + exelist[1:]
            exelist = '"%s"'%(' '.join(exelist))
            #print('\nexelist< %s >\n'%(exelist))
            # XXX: add cleanup to avoid disk space problems
            cleanstr = '"%s"'%(data.programs[config][app]['clean'])

            ## Append to experiments (must be string list)
            if action == 'profile':
                exps.append( [trialdir, '0', exelist, cleanstr] )
            else:
                exps.append( [trialdir, str(timeout), exelist, cleanstr] )
            #if verbose:
            #    print(runenv + ['-r', trialdir, str(timeout), exelist])
            #sys.exit(123)
    #print("==== experiments ====")
    #print(exps)
    #print("==== end experiments ====")
    exps = sorted( exps, key=lambda exp: float(exp[1]), reverse=True )
    return exps

def msub(nodes, partition, walltime, ntasks, env, chunk, tool, action, config, wait, nthreads, instrument, inputsize, chunkno, start, end):
    m, s = divmod(walltime, 60)
    h, m = divmod(m, 60)
    #print('%02d:%02d:%02d'%(h, m, s) )
    fname = 'submit-moab-%s-%s-%s-%s-%s-%s-%s-%s-%s-%s.sh'%(tool, action, config, wait, nthreads, instrument, inputsize, chunkno, start, end )
    print(fname)
    with open(fname, 'w') as f:
        filestr = '#!/bin/bash\n'
        filestr += '#MSUB -l nodes=' + str(nodes) + '\n'
        filestr += '#MSUB -l partition=cab\n'
        filestr += '#MSUB -l walltime=%02d:%02d:%02d\n'%( h, m, s )
        filestr += '#MSUB -q p%s\n'%( partition )
        filestr += '#MSUB -V\n'
        filestr += '#MSUB -o /usr/workspace/wsb/ggeorgak/moab.out.%j.%N\n'
        filestr += '#MSUB -e /usr/workspace/wsb/ggeorgak/moab.err.%j.%N\n'

        filestr += 'date\n'

        filestr += '$SCRIPTDIR/srun.py -N %s -p %s -n %s '%(nodes, partition, ntasks)
        filestr += ' '.join(env)
        filestr += ' ' 
        filestr += ' '.join(chunk)
        filestr += '\n'
        
        filestr += 'date\n'
        filestr += 'echo "MSUB COMPLETED"\n'

        f.write(filestr)


# TODO: put descriptive names, remove args access
def generate_jobs(nodes, partition, timelimit, exps, config, wait, tool, action, instrument, nthreads, inputsize, start, end):
    nexps = len(exps)
    if nthreads == '' or nthreads == '1':
        ntasks = 16
    elif nthreads == '8':
        ntasks = 2
    elif nthreads == '16':
        ntasks = 1
    else:
        print('Unrecognize nthreads parameter %s'%( nthreads ) )
        sys.exit(1)
    
    env=[]
    if config == 'omp':
        # ggout KMP_AFFINITY
        #env += [ '-e', 'OMP_NUM_THREADS', nthreads, '-e', 'OMP_PROC_BIND', 'close', '-e', 'OMP_WAIT_POLICY', 'passive', '-e', 'KMP_AFFINITY', 'verbose' ]
        env += [ '-e', 'OMP_NUM_THREADS', nthreads, '-e', 'OMP_PROC_BIND', 'close', '-e', 'OMP_WAIT_POLICY', wait ]
    if tool == 'safire':
        if instrument == 'all' or instrument == 'omplib':
            env += [ '-e', 'LD_LIBRARY_PATH', homedir + '/usr/local/safire/lib:' + homedir + '/usr/local/lib' ]
        else:
            env += [ '-e', 'LD_LIBRARY_PATH', homedir + '/usr/local/lib' ]
    if tool == 'refine':
        if instrument == 'all' or instrument == 'omplib':
            env += [ '-e', 'LD_LIBRARY_PATH', homedir + '/usr/local/refine/lib:' + homedir + '/usr/local/lib' ]
        else:
            env += [ '-e', 'LD_LIBRARY_PATH', homedir + '/usr/local/lib' ]

    # First Fit Decreasing bin packing
    chunks = []
    for e in exps:
        newchunk = True
        time = e[1]
        time = float(time)

        if time == 0:
            time = 3600

        for i, (t, s, c) in enumerate(chunks):
            if t + time <= timelimit:
                chunks[i] = ( t+time, s+1,  c+[e] )
                newchunk = False
        if newchunk:
            chunks.append( ( time, 1, [e] ) )
    #for t, s, c in chunks: # ggout
    #    print('===== CHUNK ====')
    #    print(c)
    #    print( 't %d len(c) %d'%( t, s ) )
    #    print('===== END CHUNK ====')
    #    break
    print( 'total %d'%( len( chunks ) ) )

    # Group by tasks in node
    for i, chunk_group in enumerate( get_chunk(chunks, nodes*ntasks) ):
        #print('=== CHUNK GROUP ===')
        #print(chunk_group)
        #print('=== END CHUNK GROUP ===')
        runlist = []
        walltime = 0
        task = 0
        # create the runlist arguments
        for t, s, clist in chunk_group:
            for ci in clist:
                runlist += ( ['-r'] + ci ) 
                task = ( task + 1 ) % ntasks
            walltime = max(walltime, t)

        #print('==== RUNLIST ====')
        #print(runlist)
        #print('==== END RUNLIST ====')

        # check if it needs all nodes
        #print('chunk_group len %d'%( len( chunk_group ) ) ) # ggout
        nodes_group = int ( len( chunk_group ) / ntasks )
        if ( len( chunk_group ) % ntasks ) > 0 :
            nodes_group += 1
        #print('nodes %d -- vs nodes_group %d'%(nodes, nodes_group) )

        # round to the next minute
        if walltime % 60 != 0:
            walltime = walltime + (60 - walltime%60)
        #print(walltime)
        print('nodes: %d'%( nodes_group ) )
        msub(nodes_group, partition, walltime, ntasks, env, runlist, tool, action, config, wait, nthreads, instrument, inputsize, i+1, start, end)
    #print('chunkno %d nexps %d'%( chunkno, len(chunk2) ) ) # ggout
    #print('Total chunks:%d'%(chunkno))
    
def get_chunk(l, n):
    """Yield successive n-sized chunks from l."""
    for i in range(0, len(l), n):
        yield l[i:i + n]

def main():
    parser = argparse.ArgumentParser('Generate moab scripts to run experiments')
    parser.add_argument('-d', '--appdir', help='applications root directory', required=True)
    parser.add_argument('-r', '--resdir', help='results directory', required=True)
    parser.add_argument('-a', '--apps', help='applications to run', choices=data.apps+['ALL'], nargs='+', required=True)
    parser.add_argument('-t', '--tools', help='tool to run', choices=['safire', 'refine', 'pinfi', 'pinfi-detach', 'golden' ], nargs='+', required=True)
    parser.add_argument('-x', '--action', help='action to take', choices=['profile', 'fi', 'fi-0', 'fi-1-15'], required=True)
    parser.add_argument('-s', '--start', help='start trial', type=int, required=True)
    parser.add_argument('-e', '--end', help='end trial', type=int, required=True)
    parser.add_argument('-c', '--mode', help='run configuration', choices=['serial', 'omp', 'ALL'], nargs='+', required=True)
    parser.add_argument('-l', '--timelimit', help='target timelimit (seconds)', type=int, required=True)
    parser.add_argument('-N', '--nodes', help='target nodes', type=int, required=True)
    parser.add_argument('-g', '--generate', help='generate moab jobscripts', default=False, action='store_true')
    parser.add_argument('-p', '--partition', help='partition to run experiments', choices=['echo', 'local', 'debug', 'batch' ], required=True)
    parser.add_argument('-w', '--wait', help='wait policy', choices=['passive', 'active'] )
    args = parser.parse_args()

    # Error checking
    assert os.path.isdir(args.appdir), 'Applications directory: ' + args.appdir + 'does not exist'
    assert args.start <= args.end, 'Start must be < end'
    assert args.timelimit > 0, 'Walltime must be > 0'
    assert args.nodes > 0, 'Node must be > 0'
    if args.apps == ['ALL']:
        args.apps = data.apps

    # fi tools
    for t in args.tools:
        for c in args.mode:
            if not c in config.data[t]['exec']:
                print('Cannot %s with %d' % ( c, t ) )
                continue
            for w in config.data[t][c]['wait']:
                for i in config.data[t][c]['inputs']:
                    for ins in config.data[t][c]['instrument']:
                        for n in config.data[t][c][i][ins]['nthreads']:
                            exps = check(args.appdir, args.resdir, t, c, w, args.apps, args.action, ins, n, i, args.start, args.end, args.generate)
                            if exps:
                                print('==== EXPERIMENT ====')
                                print( 'Experiment: %s %s %s %s %s %s %s %s %s [%s]'%( t, args.action, 
                                    c, w, n, ins, i, args.start, args.end, ', '.join(args.apps) ) )
                                #print(exps)
                                nexps = len(exps)
                                print('Nof exps: %d'%( nexps ) )
                                if args.generate:
                                    #def generate_jobs(exps, c, t, action, ins, n, i, start, end):
                                    generate_jobs(args.nodes, args.partition, args.timelimit, exps, 
                                            c, w, t, args.action, ins, n, i, args.start, args.end )
                                
                                print('==== END EXPERIMENT ====')

if __name__ == "__main__":
    main()
