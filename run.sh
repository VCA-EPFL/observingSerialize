#!/bin/bash
gcc $1.c -o $1
objdump -D $1 > $1.dump

rm log$1
touch log$1
for i in {1..100}; do
	./$1 >> log$1
done
cat log$1 | python stats.py
