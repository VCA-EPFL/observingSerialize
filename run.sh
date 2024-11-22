#!/bin/bash
gcc $1.c -o $1
objdump -D $1 > $1.dump

for i in {1..100}; do
	./$1
done
