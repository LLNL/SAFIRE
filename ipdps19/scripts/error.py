#!/usr/bin/env python3.6

import argparse
import numpy as np
import scipy.stats as stats

def margin(N, n, t, p):
    return t*np.sqrt(p*(1-p)*(N-n) /(n * (N-1)))

parser = argparse.ArgumentParser('Calculate the margin of error')
parser.add_argument('N', help='Population to sample from', type=int)
parser.add_argument('n', help='Number of samples', type=int)
parser.add_argument('ci', help='Confidence interval setting [0...1]', type=float)
parser.add_argument('p', help='Proportion of samples [0...1], 0.5 if unknown', type=float)
args = parser.parse_args()

assert args.N > 0, 'Population must be > 0!'
assert (args.N >= args.n), 'Population must be > n samples!'
assert (args.ci >= 0) and (args.ci <= 1), 'CI must be within [0...1]'
assert (args.p >= 0) and (args.p <= 1), 'Proportion must be within [0...1]'

# double-tailed interval, thus (1-ci)/2
t = stats.t.ppf(1-(1-args.ci)/2.0, args.N)
print('Margin of error: %.6f'%(margin(args.N, args.n, t, args.p)))
