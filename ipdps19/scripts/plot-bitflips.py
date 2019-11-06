#!/usr/bin/env python3.6

from bashplotlib.histogram import plot_hist
import glob
import re
import argparse

import fi_tools

parser = argparse.ArgumentParser('Run profiling experiments')
parser.add_argument('-t', '--tool', help='tool to run', required=True)
args = parser.parse_args()

bitflips=[]
for f in glob.glob('*/' + fi_tools.files[args.tool]['injection']):
    with open(f, 'r') as f:
        m = re.match('.*bitflip=(\d+).*', f.read())
        bitflips.append(m[1])

print(bitflips)
plot_hist(bitflips, title='Distro bitflip', bincount=128, xlab=True)

fi_index=[]
for f in glob.glob('*/' + fi_tools.files[args.tool]['target']):
    with open(f, 'r') as f:
        m = re.match('.*fi_index=(\d+).*', f.read())
        fi_index.append(m[1])
plot_hist(fi_index, title='Distro fi_index', bincount=128, xlab=True)

