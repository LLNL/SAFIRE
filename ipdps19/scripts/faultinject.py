#!/usr/bin/env python3

import sys
import argparse
import re
import random
import glob

parser = argparse.ArgumentParser('Create a FI target')
parser.add_argument('-e', '--execmode', help='Execution model (serial | omp | mpi | mpi+omp)', required=True)
args = parser.parse_args()

assert args.execmode in ['serial', 'omp', 'mpi', 'mpi+omp'], 'Valid execmodes are serial | omp | mpi | mpi+omp'

# parse inscount and create refine-target.txt
if args.execmode == 'serial':
    print('serial')
    with open('refine-inscount.txt','r') as f:
        m = re.match('fi_index=(\d+)', f.read())

    num_insts = int(m.group(1))
    print('num_insts %d'%num_insts)
    # +1 because it returns stop-1
    target = random.randrange(1, num_insts+1)
    print('target %d'%target)
    assert ( target >= 1) and (target <=num_insts), 'Random target %d < 1 or > num_insts %d'%(target, num_insts)

    with open('refine-target.txt', 'w') as f:
        f.write('fi_index=%d\n'%target)

elif args.execmode == 'omp':
    print('omp')
    with open('refine-inscount.txt','r') as f:
        m = re.findall('thread=(\d+), fi_index=(\d+)', f.read())
        thread_insts = [ ( int(x), int(y) ) for (x,y) in m ]

    num_insts = sum(j for __,j in thread_insts)
    print('num_insts %d'%num_insts)
    print(thread_insts) # ggout
    # +1 because it returns stop-1
    target = random.randrange(1, num_insts+1)
    print('target %d'%target)
    assert ( target >= 1) and (target <=num_insts), 'Random target %d < 1 or > num_insts %d'%(target, num_insts)

    tot_inst_count = 0
    # iterate over threads
    for thread, insts in thread_insts:
        if target <= ( tot_inst_count + insts ):
            fi_index = target - tot_inst_count
            break

        tot_inst_count += insts

    print('thread=%d, fi_index=%d\n'%( thread, fi_index ))
    with open('refine-target.txt', 'w') as f2:
        f2.write('thread=%d, fi_index=%d\n'%( thread, fi_index ))

elif args.execmode == 'mpi':
    print('mpi')
    files = glob.glob('*.refine-inscount.txt')
    rank_insts = []
    rank = 0
    for rank in range(0, len( files ) ):
        with open('%d.refine-inscount.txt'%(rank),'r') as f:
                m = re.match('\d+', f.read())
                rank_insts.append( (rank, int(m[0])) )

    num_insts = sum(j for __,j in rank_insts)
    print('num_insts %d'%num_insts)
    # +1 because it returns stop-1
    target = random.randrange(1, num_insts+1)
    print('target %d'%target)
    assert ( target >= 1) and (target <=num_insts), 'Random target %d < 1 or > num_insts %d'%(target, num_insts)

    tot_inst_count = 0
    # iterate over ranks
    for rank, insts in rank_insts:
        if target <= ( tot_inst_count + insts ):
            fi_index = target - tot_inst_count
            break

        tot_inst_count += insts

    print('rank=%d, fi_index=%d\n'%( rank, fi_index ))

    with open('refine-target.txt', 'w') as f:
        f.write('rank=%d, fi_index=%d\n'%( rank, fi_index ))
    print(rank_insts)

elif args.execmode == 'mpi+omp':
    print('mpi+omp')
    files = glob.glob('*.refine-inscount.txt')
    rank_insts = []
    rank = 0
    for rank in range(0, len( files ) ):
        with open('%d.refine-inscount.txt'%(rank),'r') as f:
                m = re.findall('thread=(\d+), fi_index=(\d+)', f.read())
                thread_insts = [ ( int(x), int(y) ) for (x,y) in m ]
                rank_insts.append( ( rank, thread_insts ) )

    print(rank_insts)
    num_insts = sum(k for i,j in rank_insts for __,k in j)
    print('num_insts %d'%num_insts)
    # +1 because it returns stop-1
    target = random.randrange(1, num_insts+1)
    print('target %d'%target)
    assert ( target >= 1) and (target <=num_insts), 'Random target %d < 1 or > num_insts %d'%(target, num_insts)
    tot_inst_count = 0
    targetFound = False
    # iterate over ranks
    for rank, thread_insts in rank_insts:
        # iterate over threads
        for thread, insts in thread_insts:
            if target <= ( tot_inst_count + insts ):
                fi_index = target - tot_inst_count
                targetFound = True
                break

            tot_inst_count += insts

        if targetFound:
            break
    print('rank=%d, thread=%d, fi_index=%d\n'%( rank, thread, fi_index ))
    with open('refine-target.txt', 'w') as f:
        f.write('rank=%d, thread=%d, fi_index=%d\n'%( rank, thread, fi_index ))

else:
    print('Invalid execmode ' + args.execmode)
    sys.exit(1)

print('Done!')


