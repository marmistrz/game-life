#!/bin/sh
if [ $# -lt 4 ]; then
    echo "Usage: $0 procs-row procs-col plane-dimension num-timesteps [mpi-args]"
    exit 1
fi
procs_row=$1
procs_col=$2
plane_dim=$3
num_steps=$4
shift 4
procs=$((procs_row * procs_col))
mpirun "$@" -n $procs ./game-life "$procs_row" "$procs_col" "$plane_dim" "$num_steps"
