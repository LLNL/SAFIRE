#!/bin/bash
# Submits moab scripts to the queue
# Q = add jobs if queued are less than Q
# S = sleep period between checking

Q=$1
S=$2

mkdir -p submitted
while true; do
    queued=$(showq -i -u ggeorgak | grep ggeorgak | wc -l)
    echo $queued
    if (( queued < $Q )); then
        echo "queued $queued < $Q"
        jobs=$(ls submit*.sh|head -n 40)
        for j in $jobs; do
            mv $j submitted
            msub submitted/$j
        done
    else
        echo "queued $queued >= $Q"
    fi
    rem=$(ls submit*.sh|wc -l)
    echo "$rem remaining"
    sleep $S
done
