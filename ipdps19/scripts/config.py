import collections

data = collections.OrderedDict( {
    'golden': {
        'exec': [ 'serial', 'omp' ],
        'serial' : {
            'wait': [ '' ],
            'inputs': [ 'small' ],
            'instrument': [ '' ],
            'small' : { 'nthreads': [ '' ] },
            },
        'omp' : {
            #'wait': [ 'active', 'passive' ],
            'wait': [ 'passive' ],
            'inputs': [ 'small', 'large' ],
            'instrument': [ '' ],
            'small' : { 'nthreads': [ '16', '8', '1' ] },
            'large' : { 'nthreads': [ '16', '8' ] },
            }
        },
    'refine': {
        #'exec': [ 'serial', 'omp' ],
        'exec': [ 'serial' ],
        'serial' : {
            'wait': [ '' ],
            'inputs': [ 'small' ],
            'instrument': [ '' ],
            'small' : { 'nthreads': [ '' ] },
            },
        # unused
        'omp' : {
            'wait': [ 'active', 'passive' ],
            'inputs': [ 'small' ],
            'instrument': [ 'all', 'app', 'omplib' ],
            'small' : { 'nthreads': [ '16', '8', '1' ] },
            #'large' : { 'nthreads': [ '16', '8' ] },
            }
        },
    'pinfi': {
        'exec': [ 'serial', 'omp' ],
        'serial' : {
            'wait': [ '' ],
            'inputs': [ 'small' ],
            'instrument': [ '' ],
            'small' : { 'nthreads': [ '' ] },
            },
        'omp' : {
            'wait': [ 'active', 'passive' ],
            'inputs': [ 'small' ],
            'instrument': [ 'all', 'app', 'omplib' ],
            'small' : { 'nthreads': [ '16', '8', '1' ] },
            #'large' : { 'nthreads': [ '16', '8' ] },
            }
        },
    'pinfi-detach': {
        'exec': [ 'serial', 'omp' ],
        'serial' : {
            'wait': [ '' ],
            'inputs': [ 'small' ],
            'instrument': [ '' ],
            'small' : { 'nthreads': [ '' ] },
            },
        'omp' : {
            #'wait': [ 'active', 'passive' ],
            'wait': [ 'passive' ],
            #'inputs': [ 'small', 'large' ],
            'inputs': [ 'small' ],
            #'instrument': [ 'all', 'app', 'omplib' ],
            'instrument': [ 'all' ],
            #'small' : { 'nthreads': [ '16', '8', '1' ] },
            'small' : { 'nthreads': [ '16' ] },
            'large' : { 'nthreads': [ '16', '8' ] },
            }
        },
    'safire': {
        'exec': [ 'serial', 'omp' ],
        'serial' : {
            'wait': [ '' ],
            'inputs': [ 'small' ],
            'instrument': [ '' ],
            'small' : { 'nthreads': [ '' ] },
            },
        'omp' : {
            #'wait': [ 'active', 'passive' ],
            'wait': [ 'passive' ],
            'inputs': [ 'small', 'large' ],
            'instrument': [ 'all', 'app', 'omplib' ],
            #'small' : { 'nthreads': [ '16', '8', '1' ] },
            'small' : { 'nthreads': [ '16', '8' ] },
            'large' : { 'nthreads': [ '16', '8' ] },
            },
        },
    } )
