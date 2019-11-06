#!/usr/bin/env python3.6

import argparse
import numpy as np
import scipy.stats as stats

def nsamples(N, e, t, p):
    return N / ( 1 + e**2 * ( ( N - 1) / ( t**2 * p * ( 1 - p) ) ) )

parser = argparse.ArgumentParser('Calculate the margin of error')
parser.add_argument('N', help='Population to sample from', type=int)
parser.add_argument('e', help='Margin of error', type=float)
parser.add_argument('ci', help='Confidence interval setting [0...1]', type=float)
parser.add_argument('p', help='Proportion of samples [0...1], 0.5 if unknown', type=float)
args = parser.parse_args()

assert args.N > 0, 'Population must be > 0!'
assert (args.e >= 0) and (args.e <=1), 'Margin of error must be within [0...1]'
assert (args.ci >= 0) and (args.ci <= 1), 'CI must be within [0...1]'
assert (args.p >= 0) and (args.p <= 1), 'Proportion must be within [0...1]'

# double-tailed interval, thus (1-ci)/2
t = stats.t.ppf(1-(1-args.ci)/2.0, args.N)
print('Number of samples needed: %.6f'%(nsamples(args.N, args.e, t, args.p)))
