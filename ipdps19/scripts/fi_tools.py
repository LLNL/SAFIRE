import data
import sys
import random
import os
import re
import subprocess

try:
    pinroot = os.environ['PIN_ROOT']
except:
    print('Env variable PIN_ROOT is missing')
    sys.exit(1)

try:
    pinfidir = os.environ['PINFI']
except:
    print('Env variable PINFI is missing')
    sys.exit(1)


pinbin = pinroot + '/pin'

files = {
        'pinfi': {
            'inscount': 'pin.instcount.txt',
            'injection':'pin.injection.txt',
            'target':'pin.target.txt',
            },
        'pinfi-detach': {
            'inscount': 'pin.instcount.txt',
            'injection':'pin.injection.txt',
            'target':'pin.target.txt',
            },
        'safire': {
            'inscount':'refine-inscount.txt',
            'injection':'refine-inject.txt',
            'target':'refine-target.txt'
        },
        'refine': {
            'inscount':'refine-inscount.txt',
            'injection':'refine-inject.txt',
            'target':'refine-target.txt'
        }
    }

exelist = {
    'pinfi':  {
        'serial': {
            'profile':[pinbin, '-t', pinfidir + '/obj-intel64/instcount', '--'],
            'fi'     :[pinbin, '-t', pinfidir + '/obj-intel64/faultinjection', '--'],
        },
        'omp': {
           'profile': {
                'all'   :[pinbin, '-t', pinfidir + '/obj-intel64/instcount_omp', '-instr-libs', 'libomp', '--'],
                'app'   :[pinbin, '-t', pinfidir + '/obj-intel64/instcount_omp', '--'],
                'omplib':[pinbin, '-t', pinfidir + '/obj-intel64/instcount_omp', '-nomain', '-instr-libs', 'libomp', '--'],
            },
            'fi': {
                'all'   :[pinbin, '-t', pinfidir + '/obj-intel64/faultinjection_omp', '-instr-libs', 'libomp', '--'],
                'app'   :[pinbin, '-t', pinfidir + '/obj-intel64/faultinjection_omp', '--'],
                'omplib':[pinbin, '-t', pinfidir + '/obj-intel64/faultinjection_omp', '-nomain', '-instr-libs', 'libomp', '--']
            }
        }
    },
    'pinfi-detach':  {
        'serial': {
            'profile':[pinbin, '-t', pinfidir + '/obj-intel64/instcount', '--'],
            'fi'     :[pinbin, '-t', pinfidir + '/obj-intel64/faultinjection', '-detach', '--'],
        },
        'omp': {
           'profile': {
                'all'   :[pinbin, '-t', pinfidir + '/obj-intel64/instcount_omp', '-instr-libs', 'libomp', '--'],
                'app'   :[pinbin, '-t', pinfidir + '/obj-intel64/instcount_omp', '--'],
                'omplib':[pinbin, '-t', pinfidir + '/obj-intel64/instcount_omp', '-nomain', '-instr-libs', 'libomp', '--'],
            },
            'fi': {
                'all'   :[pinbin, '-t', pinfidir + '/obj-intel64/faultinjection_omp', '-detach', '-instr-libs', 'libomp', '--'],
                'app'   :[pinbin, '-t', pinfidir + '/obj-intel64/faultinjection_omp', '-detach', '--'],
                'omplib':[pinbin, '-t', pinfidir + '/obj-intel64/faultinjection_omp', '-detach', '-nomain', '-instr-libs', 'libomp', '--']
            }
        }
    },
}
