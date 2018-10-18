#!/bin/sh
set -e
set -o pipefail

run() {
   ./run.sh "$1" "$2" 360 5 --oversubscribe | sed '6q;d'
}

val=$(run 1 1)
for i in $(seq 1 3); do
    for j in $(seq 1 3); do
        val2=$(run "$i" "$j")
        if [ "$val" != "$val2" ]; then
            echo "Error: sequential run yielded $val, parallel $i x $j yielded $val2"
            exit 1
        else 
            echo "Grid $i x $j ok..."
        fi
    done
done
echo "All tests passed"