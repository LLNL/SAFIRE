import os
import sys

#Programs
## XXX: IS is very short running and it has no output to verify
# TODO: FIX UA, RE-RUN UA experiments for small input, changed to A. Possible large too because removing
apps = [ 'AMG' , 'CoMD', 'HPCCG-1.0', 'lulesh', 'XSBench', 'miniFE', 'BT', 'CG', 'DC', 'EP', 'FT', 'LU', 'MG', 'SP', 'UA' ]
inputs = [ 'test', 'small', 'large' ]
configs = [ 'serial', 'omp' ]

# NAS base directories
NASDIR_SER = 'NPB3.3-SER-C/'
NASDIR_OMP = 'NPB3.3-OMP-C/'

# FP formats for verifying
#FLOAT = r'[+-]?\d+\.(?:\d+|\d+[Ee][+-]?\d+)'
FLOAT = r'[+-]?\d+\.\d+(?:[Ee][+-]?\d+)?'

# builddir: where to run make
# rundir: where to run experiments
# exec: pre-pend first element with appdir in run scripts
programs = {
    'serial' : {
        'AMG' : {
            'builddir':'AMG/', 'buildargs': { 'test':[], 'small':[], 'large':[] },
            'exec' : {
                'test'  : ['/AMG/test/amg', '-n', '32', '32', '32'],
                'small' : ['/AMG/test/amg', '-n', '96', '96', '96'],
                'large' : ['/AMG/test/amg', '-n', '256', '256', '256'],
            },
            'clean' : '',
            'verify' : {
                'test'  :['Final Relative Residual Norm = ' + FLOAT],
                'small' :['Final Relative Residual Norm = ' + FLOAT],
                'large' :['Final Relative Residual Norm = ' + FLOAT],
            }
        },

        'CoMD' : {
            'builddir':'CoMD'+'/src-mpi/', 'buildargs': { 'test':[], 'small':[], 'large':[] },
            'exec' : {
                # XXX: $APPDIR should be replaced by srun script
                'test'  : ['/CoMD/bin/CoMD-serial', '-d', '$APPDIR/pots/', '-e', '-i', '1', '-j', '1', '-k', '1', '-x', '4', '-y', '4', '-z', '4'],
                'small' : ['/CoMD/bin/CoMD-serial', '-d', '$APPDIR/pots/', '-e', '-i', '1', '-j', '1', '-k', '1', '-x', '32', '-y', '32', '-z', '32'],
                'large' : ['/CoMD/bin/CoMD-serial', '-d', '$APPDIR/pots/', '-e', '-i', '1', '-j', '1', '-k', '1', '-x', '64', '-y', '64', '-z', '64']
            },
            'clean' : '/bin/rm -rf CoMD*yaml',
            'verify' : {
                'test' : ['Final energy\s+: ' + FLOAT, 'Final atom count : \d+, no atoms lost' ],
                'small': ['Final energy\s+: ' + FLOAT, 'Final atom count : \d+, no atoms lost' ],
                'large': ['Final energy\s+: ' + FLOAT, 'Final atom count : \d+, no atoms lost' ],
            }
        },

        'HPCCG-1.0' : {
            'builddir':'HPCCG-1.0/', 'buildargs': { 'test':[], 'small':[], 'large':[] },
            'exec' : {
                'test' : ['HPCCG-1.0/test_HPCCG', '32', '32', '32'],
                'small': ['HPCCG-1.0/test_HPCCG', '128', '128', '128'],
                'large': ['HPCCG-1.0/test_HPCCG', '256', '256', '256']
            },
            'clean' : '/bin/rm -rf hpccg*yaml',
            'verify' : {
                'test'  : ['Final residual: : ' + FLOAT ],
                'small' : ['Final residual: : ' + FLOAT ],
                'large' : ['Final residual: : ' + FLOAT ],
            }
        },

        'lulesh' : {
            'builddir':'lulesh/', 'buildargs': { 'test':[], 'small':[], 'large':[] },
            'exec' : {
                'test'  : ['lulesh/lulesh2.0', '-i', '10'],
                'small' : ['lulesh/lulesh2.0'],
                'large' : ['lulesh/lulesh2.0', '-s', '50'],
            },
            'clean' : '',
            'verify' : {
                'test'  : ['Final Origin Energy = ' + FLOAT ],
                'small' : ['Final Origin Energy = ' + FLOAT ],
                'large' : ['Final Origin Energy = ' + FLOAT ],
            }
        },

        'XSBench' : {
            'builddir':'XSBench/src/', 'buildargs': { 'test':[], 'small':[], 'large':[] },
            'exec' : {
                'test'  : ['/XSBench/src/XSBench','-s','small', '-l', '100000'],
                'small' : ['/XSBench/src/XSBench','-s','small'],
                'large' : ['/XSBench/src/XSBench','-s','large'],
            },
            'clean' : '',
            'verify' : {
                'test'  : ['Verification checksum: 500568216'],
                'small' : ['Verification checksum: 74966788162'],
                'large' : ['Verification checksum: 74994938929']
            }
        },

        'miniFE' : {
            'builddir':'miniFE/ref/src/', 'buildargs': { 'test':[], 'small':[], 'large':[] },
            'exec' : {
                #'test'  : ['/miniFE/ref/src/miniFE.x','-nx','16','-ny','16','-nz','16','-verify_solution','1'],
                #'small' : ['/miniFE/ref/src/miniFE.x','-nx','64','-ny','64','-nz','64','-verify_solution','1'],
                #'large' : ['/miniFE/ref/src/miniFE.x','-nx','256','-ny','256','-nz','256','-verify_solution','1'],
                'test'  : ['/miniFE/ref/src/miniFE.x','-nx','16','-ny','16','-nz','16'],
                'small' : ['/miniFE/ref/src/miniFE.x','-nx','64','-ny','64','-nz','64'],
                'large' : ['/miniFE/ref/src/miniFE.x','-nx','256','-ny','256','-nz','256'],
            },
            'clean' : '/bin/rm -rf miniFE*yaml',
            'verify' : {
                'test'  : ['Final Resid Norm: ' + FLOAT],
                #'small'  : [ 'solution matches' ],
                'small' : ['Final Resid Norm: ' + FLOAT],
                'large' : ['Final Resid Norm: ' + FLOAT],
            }
        },

        'BT': {
            'rundir':NASDIR_SER+'BT/','builddir':NASDIR_SER,'buildargs':{'test': ['BT','CLASS=S'], 'small': ['BT','CLASS=A'], 'large': ['BT', 'CLASS=B'] },
            'exec' : {
                'test'  : [NASDIR_SER+'bin/bt.S.x'],
                'small' : [NASDIR_SER+'bin/bt.A.x'],
                'large' : [NASDIR_SER+'bin/bt.B.x'],
            },
            'clean' : '',
            'verify' : {
                'test' :['\d\s+(' + FLOAT +') ' + FLOAT + ' ' + FLOAT],
                'small':['\d\s+(' + FLOAT +') ' + FLOAT + ' ' + FLOAT],
                'large':['\d\s+(' + FLOAT +') ' + FLOAT + ' ' + FLOAT],
            }
        },
        'CG': {
            'rundir':NASDIR_SER+'CG/','builddir':NASDIR_SER,'buildargs':{'test': ['CG','CLASS=S'], 'small': ['CG','CLASS=B'], 'large': ['CG', 'CLASS=C'] },
            'exec' : {
                'test'  : [NASDIR_SER+'bin/cg.S.x'],
                'small' : [NASDIR_SER+'bin/cg.B.x'],
                'large' : [NASDIR_SER+'bin/cg.C.x'],
            },
            'clean' : '',
            'verify' : {
                'test' :[' Zeta is\s+' + FLOAT],
                'small':[' Zeta is\s+' + FLOAT],
                'large':[' Zeta is\s+' + FLOAT],
            }
        },
        'DC': {
            'rundir':NASDIR_SER+'DC/','builddir':NASDIR_SER,'buildargs':{'test': ['DC','CLASS=S'], 'small': ['DC','CLASS=W'], 'large': ['DC', 'CLASS=A'] },
            'exec' : {
                'test'  : [NASDIR_SER+'bin/dc.S.x'],
                'small' : [NASDIR_SER+'bin/dc.W.x'],
                'large' : [NASDIR_SER+'bin/dc.A.x'],
            },
            'clean' : '/bin/rm -rf ADC.*',
            'verify' : {
                'test' :['Checksum\s+=\s+' + FLOAT],
                'small':['Checksum\s+=\s+' + FLOAT],
                'large':['Checksum\s+=\s+' + FLOAT],
            }
        },
        'EP': {
            'rundir':NASDIR_SER+'EP/','builddir':NASDIR_SER,'buildargs':{'test': ['EP','CLASS=S'], 'small': ['EP','CLASS=A'], 'large': ['EP', 'CLASS=C'] },
            'exec' : {
                'test'  : [NASDIR_SER+'bin/ep.S.x'],
                'small' : [NASDIR_SER+'bin/ep.A.x'],
                'large' : [NASDIR_SER+'bin/ep.C.x'],
            },
            'clean' : '',
            'verify' : {
                'test' :['Sums =\s+' + FLOAT + '\s+' + FLOAT],
                'small':['Sums =\s+' + FLOAT + '\s+' + FLOAT],
                'large':['Sums =\s+' + FLOAT + '\s+' + FLOAT],
            }
        },
        'FT': {
            'rundir':NASDIR_SER+'FT/','builddir':NASDIR_SER,'buildargs':{'test': ['FT','CLASS=S'], 'small': ['FT','CLASS=B'], 'large': ['FT', 'CLASS=B'] },
            'exec' : {
                'test'  : [NASDIR_SER+'bin/ft.S.x'],
                'small' : [NASDIR_SER+'bin/ft.B.x'],
                'large' : [NASDIR_SER+'bin/ft.B.x'],
            },
            'clean' : '',
            'verify' : {
                'test' :['T =\s+\d+\s+Checksum =\s+' + FLOAT + '\s+' + FLOAT],
                'small':['T =\s+\d+\s+Checksum =\s+' + FLOAT + '\s+' + FLOAT],
                'large':['T =\s+\d+\s+Checksum =\s+' + FLOAT + '\s+' + FLOAT],
            }
        },
        'IS': {
            'rundir':NASDIR_SER+'IS/','builddir':NASDIR_SER,'buildargs':{'test': ['IS','CLASS=S'], 'small': ['IS','CLASS=A'], 'large': ['IS', 'CLASS=A'] },
            'exec' : {
                'test'  : [NASDIR_SER+'bin/is.S.x'],
                'small' : [NASDIR_SER+'bin/is.A.x'],
                'large' : [NASDIR_SER+'bin/is.A.x'],
            },
            'clean' : '',
            'verify' : {
                # XXX: no way to verify IS
            }
        },
        'LU': {
            'rundir':NASDIR_SER+'LU/','builddir':NASDIR_SER,'buildargs':{'test': ['LU','CLASS=S'], 'small': ['LU','CLASS=A'], 'large': ['LU', 'CLASS=C'] },
            'exec' : {
                'test'  : [NASDIR_SER+'bin/lu.S.x'],
                'small' : [NASDIR_SER+'bin/lu.A.x'],
                'large' : [NASDIR_SER+'bin/lu.C.x'],
            },
            'clean' : '',
            'verify' : {
                'test' : ['\d?\s+(' + FLOAT +') ' + FLOAT + ' ' + FLOAT],
                'small': ['\d?\s+(' + FLOAT +') ' + FLOAT + ' ' + FLOAT],
                'large': ['\d?\s+(' + FLOAT +') ' + FLOAT + ' ' + FLOAT],
            }
        },
        'MG': {
            'rundir':NASDIR_SER+'MG/','builddir':NASDIR_SER,'buildargs':{'test': ['MG','CLASS=S'], 'small': ['MG','CLASS=B'], 'large': ['MG', 'CLASS=C'] },
            'exec' : {
                'test'  : [NASDIR_SER+'bin/mg.S.x'],
                'small' : [NASDIR_SER+'bin/mg.B.x'],
                'large' : [NASDIR_SER+'bin/mg.C.x'],
            },
            'clean' : '',
            'verify' : {
            #    **dict.fromkeys( ['test', 'small', 'large'], ['\n L2 Norm is\s+' + FLOAT ] )
                'test' : ['\n L2 Norm is\s+' + FLOAT ],
                'small': ['\n L2 Norm is\s+' + FLOAT ],
                'large': ['\n L2 Norm is\s+' + FLOAT ],
            }
        },
        'SP': {
            'rundir':NASDIR_SER+'SP/','builddir':NASDIR_SER,'buildargs':{'test': ['SP','CLASS=S'], 'small': ['SP','CLASS=A'], 'large': ['SP', 'CLASS=B'] },
            'exec' : {
                'test'  : [NASDIR_SER+'bin/sp.S.x'],
                'small' : [NASDIR_SER+'bin/sp.A.x'],
                'large' : [NASDIR_SER+'bin/sp.B.x'],
            },
            'clean' : '',
            'verify' : {
                'test' :['\d\s+(' + FLOAT +') ' + FLOAT + ' ' + FLOAT],
                'small':['\d\s+(' + FLOAT +') ' + FLOAT + ' ' + FLOAT],
                'large':['\d\s+(' + FLOAT +') ' + FLOAT + ' ' + FLOAT],
            }
        },
        'UA': {
            'rundir':NASDIR_SER+'UA/','builddir':NASDIR_SER,'buildargs':{'test': ['UA','CLASS=S'], 'small': ['UA','CLASS=A'], 'large': ['UA', 'CLASS=B'] },
            'exec' : {
                'test'  : [NASDIR_SER+'bin/ua.S.x'],
                'small' : [NASDIR_SER+'bin/ua.A.x'],
                'large' : [NASDIR_SER+'bin/ua.B.x'],
            },
            'clean' : '',
            'verify' : {
                'test' :['\s+(' + FLOAT +') ' + FLOAT + ' ' + FLOAT, ],
                'small':['\s+(' + FLOAT +') ' + FLOAT + ' ' + FLOAT, ],
                'large':['\s+(' + FLOAT +') ' + FLOAT + ' ' + FLOAT, ],
            }
        }
    },
    'omp' : {
        'AMG' : {
            'builddir':'AMG/', 'buildargs': { 'test':[], 'small':[], 'large':[] },
            'exec' : {
                'test'  : ['/AMG/test/amg', '-n', '32', '32', '32'],
                'small' : ['/AMG/test/amg', '-n', '96', '96', '96'],
                'large' : ['/AMG/test/amg', '-n', '256', '256', '256'],
            },
            'clean' : '',
            'verify' : {
                'test'  :['Final Relative Residual Norm = ' + FLOAT],
                'small' :['Final Relative Residual Norm = ' + FLOAT],
                'large' :['Final Relative Residual Norm = ' + FLOAT],
            }
        },

        'CoMD' : {
            'builddir':'CoMD'+'/src-openmp/', 'buildargs': { 'test':[], 'small':[], 'large':[] },
            'exec' : {
                # XXX: $RUNDIR should be replaced by srun script
                'test'  : ['/CoMD/bin/CoMD-openmp', '-d', '$APPDIR/pots/', '-e', '-i', '1', '-j', '1', '-k', '1', '-x', '4', '-y', '4', '-z', '4'],
                'small' : ['/CoMD/bin/CoMD-openmp', '-d', '$APPDIR/pots/', '-e', '-i', '1', '-j', '1', '-k', '1', '-x', '32', '-y', '32', '-z', '32'],
                'large' : ['/CoMD/bin/CoMD-openmp', '-d', '$APPDIR/pots/', '-e', '-i', '1', '-j', '1', '-k', '1', '-x', '64', '-y', '64', '-z', '64']
            },
            'clean' : '/bin/rm -rf CoMD*yaml',
            'verify' : {
                'test' : ['Final energy\s+: ' + FLOAT, 'Final atom count : \d+, no atoms lost' ],
                'small': ['Final energy\s+: ' + FLOAT, 'Final atom count : \d+, no atoms lost' ],
                'large': ['Final energy\s+: ' + FLOAT, 'Final atom count : \d+, no atoms lost' ],
            }
        },

        'HPCCG-1.0' : {
            'builddir':'HPCCG-1.0/', 'buildargs': { 'test':[], 'small':[], 'large':[] },
            'exec' : {
                'test'  : ['HPCCG-1.0/test_HPCCG', '32', '32', '32'],
                'small' : ['HPCCG-1.0/test_HPCCG', '128', '128', '128'],
                'large' : ['HPCCG-1.0/test_HPCCG', '256', '256', '256']
            },
            'clean' : '/bin/rm -rf hpccg*yaml',
            'verify' : {
                'test'  : ['Final residual: : ' + FLOAT ],
                'small' : ['Final residual: : ' + FLOAT ],
                'large' : ['Final residual: : ' + FLOAT ],
            }
        },

        'lulesh' : {
            'builddir':'lulesh/', 'buildargs': { 'test':[], 'small':[], 'large':[] },
            'exec' : {
                'test'  : ['lulesh/lulesh2.0', '-i', '10'],
                'small' : ['lulesh/lulesh2.0'],
                'large' : ['lulesh/lulesh2.0', '-s', '50'],
            },
            'clean' : '',
            'verify' : {
                'test'  : ['Final Origin Energy = ' + FLOAT ],
                'small' : ['Final Origin Energy = ' + FLOAT ],
                'large' : ['Final Origin Energy = ' + FLOAT ],
            }
        },
        # XXX: XSBench needs the number of threads
        'XSBench' : {
            'builddir':'XSBench/src/', 'buildargs': { 'test':[], 'small':[], 'large':[] },
            'exec' : {
                'test'  : ['/XSBench/src/XSBench','-s','small', '-l', '100000',',-t','$NTHREADS'],
                'small' : ['/XSBench/src/XSBench','-s','small','-t','$NTHREADS'],
                'large' : ['/XSBench/src/XSBench','-s','large','-t','$NTHREADS'],
            },
            'clean' : '',
            'verify' : {
                'test'  : ['Verification checksum: 500568216'],
                'small' : ['Verification checksum: 74966788162'],
                'large' : ['Verification checksum: 74994938929']
            }
        },

        'miniFE' : {
            'builddir':'miniFE/openmp-opt/src/', 'buildargs': { 'test':[], 'small':[], 'large':[] },
            'exec' : {
                'test'  : ['/miniFE/openmp-opt/src/miniFE.x','-nx','16','-ny','16','-nz','16','-verify_solution','1'],
                'small' : ['/miniFE/openmp-opt/src/miniFE.x','-nx','64','-ny','64','-nz','64','-verify_solution','1'],
                'large' : ['/miniFE/openmp-opt/src/miniFE.x','-nx','256','-ny','256','-nz','256','-verify_solution','1'],
            },
            'clean' : '/bin/rm -rf miniFE*yaml',
            'verify' : {
                'test' : ['Final Resid Norm: ' + FLOAT],
                #'small'  : [ 'solution matches' ],
                'small' : ['Final Resid Norm: ' + FLOAT],
                #'large'  : [ 'solution matches' ],
                'large' : ['Final Resid Norm: ' + FLOAT],
            }
        },

        'BT': {
            'rundir':NASDIR_OMP+'BT/','builddir':NASDIR_OMP,'buildargs':{'test': ['BT','CLASS=S'], 'small': ['BT','CLASS=A'], 'large': ['BT', 'CLASS=B'] },
            'exec' : {
                'test'  : [NASDIR_OMP+'bin/bt.S.x'],
                'small' : [NASDIR_OMP+'bin/bt.A.x'],
                'large' : [NASDIR_OMP+'bin/bt.B.x'],
            },
            'clean' : '',
            'verify' : {
                'test' :['\d\s+(' + FLOAT +') ' + FLOAT + ' ' + FLOAT],
                'small':['\d\s+(' + FLOAT +') ' + FLOAT + ' ' + FLOAT],
                'large':['\d\s+(' + FLOAT +') ' + FLOAT + ' ' + FLOAT],
            }
        },
        'CG': {
            'rundir':NASDIR_OMP+'CG/','builddir':NASDIR_OMP,'buildargs':{'test': ['CG','CLASS=S'], 'small': ['CG','CLASS=B'], 'large': ['CG', 'CLASS=C'] },
            'exec' : {
                'test'  : [NASDIR_OMP+'bin/cg.S.x'],
                'small' : [NASDIR_OMP+'bin/cg.B.x'],
                'large' : [NASDIR_OMP+'bin/cg.C.x'],
            },
            'clean' : '',
            'verify' : {
                'test' :[' Zeta is\s+' + FLOAT],
                'small':[' Zeta is\s+' + FLOAT],
                'large':[' Zeta is\s+' + FLOAT],
            }
        },
        'DC': {
            'rundir':NASDIR_OMP+'DC/','builddir':NASDIR_OMP,'buildargs':{'test': ['DC','CLASS=S'], 'small': ['DC','CLASS=W'], 'large': ['DC', 'CLASS=A'] },
            'exec' : {
                'test'  : [NASDIR_OMP+'bin/dc.S.x'],
                'small' : [NASDIR_OMP+'bin/dc.W.x'],
                'large' : [NASDIR_OMP+'bin/dc.A.x'],
            },
            'clean' : '/bin/rm -rf ADC.*',
            'verify' : {
                'test' :['Checksum\s+=\s+' + FLOAT],
                'small':['Checksum\s+=\s+' + FLOAT],
                'large':['Checksum\s+=\s+' + FLOAT],
            }
        },
        'EP': {
            'rundir':NASDIR_OMP+'EP/','builddir':NASDIR_OMP,'buildargs':{'test': ['EP','CLASS=S'], 'small': ['EP','CLASS=A'], 'large': ['EP', 'CLASS=C'] },
            'exec' : {
                'test'  : [NASDIR_OMP+'bin/ep.S.x'],
                'small' : [NASDIR_OMP+'bin/ep.A.x'],
                'large' : [NASDIR_OMP+'bin/ep.C.x'],
            },
            'clean' : '',
            'verify' : {
                'test' :['Sums =\s+' + FLOAT + '\s+' + FLOAT],
                'small':['Sums =\s+' + FLOAT + '\s+' + FLOAT],
                'large':['Sums =\s+' + FLOAT + '\s+' + FLOAT],
            }
        },
        'FT': {
            'rundir':NASDIR_OMP+'FT/','builddir':NASDIR_OMP,'buildargs':{'test': ['FT','CLASS=S'], 'small': ['FT','CLASS=B'], 'large': ['FT', 'CLASS=B'] },
            'exec' : {
                'test'  : [NASDIR_OMP+'bin/ft.S.x'],
                'small' : [NASDIR_OMP+'bin/ft.B.x'],
                'large' : [NASDIR_OMP+'bin/ft.B.x'],
            },
            'clean' : '',
            'verify' : {
                'test' :['T =\s+\d+\s+Checksum =\s+' + FLOAT + '\s+' + FLOAT],
                'small':['T =\s+\d+\s+Checksum =\s+' + FLOAT + '\s+' + FLOAT],
                'large':['T =\s+\d+\s+Checksum =\s+' + FLOAT + '\s+' + FLOAT],
            }
        },
        'IS': {
            'rundir':NASDIR_OMP+'IS/','builddir':NASDIR_OMP,'buildargs':{'test': ['IS','CLASS=S'], 'small': ['IS','CLASS=A'], 'large': ['IS', 'CLASS=A'] },
            'exec' : {
                'test'  : [NASDIR_OMP+'bin/is.S.x'],
                'small' : [NASDIR_OMP+'bin/is.A.x'],
                'large' : [NASDIR_OMP+'bin/is.A.x'],
            },
            'clean' : '',
            'verify' : {
                # XXX: no way to verify IS
            }
        },
        'LU': {
            'rundir':NASDIR_OMP+'LU/','builddir':NASDIR_OMP,'buildargs':{'test': ['LU','CLASS=S'], 'small': ['LU','CLASS=A'], 'large': ['LU', 'CLASS=C'] },
            'exec' : {
                'test'  : [NASDIR_OMP+'bin/lu.S.x'],
                'small' : [NASDIR_OMP+'bin/lu.A.x'],
                'large' : [NASDIR_OMP+'bin/lu.C.x'],
            },
            'clean' : '',
            'verify' : {
                'test' : ['\d?\s+(' + FLOAT +') ' + FLOAT + ' ' + FLOAT],
                'small': ['\d?\s+(' + FLOAT +') ' + FLOAT + ' ' + FLOAT],
                'large': ['\d?\s+(' + FLOAT +') ' + FLOAT + ' ' + FLOAT],
            }
        },
        'MG': {
            'rundir':NASDIR_OMP+'MG/','builddir':NASDIR_OMP,'buildargs':{'test': ['MG','CLASS=S'], 'small': ['MG','CLASS=B'], 'large': ['MG', 'CLASS=C'] },
            'exec' : {
                'test'  : [NASDIR_OMP+'bin/mg.S.x'],
                'small' : [NASDIR_OMP+'bin/mg.B.x'],
                'large' : [NASDIR_OMP+'bin/mg.C.x'],
            },
            'clean' : '',
            'verify' : {
                'test' : ['\n L2 Norm is\s+' + FLOAT ],
                'small': ['\n L2 Norm is\s+' + FLOAT ],
                'large': ['\n L2 Norm is\s+' + FLOAT ],
            }
        },
        'SP': {
            'rundir':NASDIR_OMP+'SP/','builddir':NASDIR_OMP,'buildargs':{'test': ['SP','CLASS=S'], 'small': ['SP','CLASS=A'], 'large': ['SP', 'CLASS=B'] },
            'exec' : {
                'test'  : [NASDIR_OMP+'bin/sp.S.x'],
                'small' : [NASDIR_OMP+'bin/sp.A.x'],
                'large' : [NASDIR_OMP+'bin/sp.B.x'],
            },
            'clean' : '',
            'verify' : {
                'test' :['\d\s+(' + FLOAT +') ' + FLOAT + ' ' + FLOAT],
                'small':['\d\s+(' + FLOAT +') ' + FLOAT + ' ' + FLOAT],
                'large':['\d\s+(' + FLOAT +') ' + FLOAT + ' ' + FLOAT],
            }
        },
        'UA': {
            'rundir':NASDIR_OMP+'UA/','builddir':NASDIR_OMP,'buildargs':{'test': ['UA','CLASS=S'], 'small': ['UA','CLASS=A'], 'large': ['UA', 'CLASS=B'] },
            'exec' : {
                'test'  : [NASDIR_OMP+'bin/ua.S.x'],
                'small' : [NASDIR_OMP+'bin/ua.A.x'],
                'large' : [NASDIR_OMP+'bin/ua.B.x'],
            },
            'clean' : '',
            'verify' : {
                'test' :['\s+(' + FLOAT +') ' + FLOAT + ' ' + FLOAT, ],
                'small':['\s+(' + FLOAT +') ' + FLOAT + ' ' + FLOAT, ],
                'large':['\s+(' + FLOAT +') ' + FLOAT + ' ' + FLOAT, ],
            }
        }
    }
}


