#!/bin/bash

# Visualize better the values that a process got and sent to their neighbours #
# Execution: ch.sh $1, where $1 is a file containing a matrix #

input=$1 # file to print

#set colors
DEF='\033[0m' # no color
RED='\033[0;31m'
GRN='\033[0;32m' # green
BRN='\033[0;33m' # brown
BLUE='\033[0;34m' # blue
PRL='\033[0;35m' # purple
CYAN='\033[0;36m' # cyan
LRD='\033[1;31m' # light red
LGR='\033[1;32m' # light green
YLL='\033[1;33m' # yellow
LPR='\033[1;35m' # light purple
# num of rows
NR=`(cat $input | wc -l)`
NR=$((NR))

# num of columns
NC=`(head -n 1 $input | wc -w)`
NC=$((NC))

# current row
i=-1
while IFS= read -r line
do
	i=$((i+1))
	j=0 # current column

	for num in $line; do
		num=$((num))
		if [ $i -eq 0 -a $j -eq 0 ] || [ $i -eq $((NR - 2)) -a $j -eq $((NC - 2)) ]
		then
			echo -e -n "${RED}$num${DEF}\t"
		elif [ $i -eq 0 -a $j -eq $((NC - 1)) ] || [ $i -eq $((NR - 2)) -a $j -eq 1 ]
		then
			echo -e -n "${LPR}$num${DEF}\t"
		elif [ $i -eq 0 -a $j -ne $((NC - 1)) -a $j -gt 0 ] || [ $i -eq $((NR - 2)) -a $j -ne 0 -a $j -lt $((NC - 2)) ]
		then
			echo -e -n "${LRD}$num${DEF}\t"
		elif [ $j -eq 1 -a $i -gt 1 -a $i -lt $((NR - 2)) ] || [ $j -eq $((NC - 1)) -a $i -gt 0 -a $i -lt $((NR - 1)) ]
		then
			echo -e -n "${BLUE}$num${DEF}\t"
		elif [ $j -eq 1 -a $i -eq 1 ] || [ $i -eq $((NR - 1)) -a $j -eq $((NC - 1)) ]
		then
			echo -e -n "${PRL}$num${DEF}\t"
		elif [ $i -eq 1 -a $j -gt 1 -a $j -lt $((NC - 2)) ] || [ $i -eq $((NR - 1)) -a $j -gt 0 -a $j -lt $((NC - 1)) ]
		then
			echo -e -n "${CYAN}$num${DEF}\t"
		elif [ $i -eq 1 -a $j -eq $((NC - 2)) ] || [ $i -eq $((NR - 1)) -a $j -eq 0 ]
		then
			echo -e -n "${BRN}$num${DEF}\t"
		elif [ $j -eq 0 -a $i -gt 0 -a $i -lt $((NR - 1)) ] || [ $j -eq $((NC - 2)) -a $i -gt 0 -a $i -lt $((NR - 1)) ]
		then
			echo -e -n "${GRN}$num${DEF}\t"
		else
			echo -e -n "$num\t"
		fi
		j=$((j+1))
	done
		printf "\n"
done < "$input"
