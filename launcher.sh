#!/bin/sh

numa_nodes=$(lscpu | awk '/NUMA node\(/{print $3}')
outfile=$(mktemp)
testfile=testfile
iterations=20

[ -z "$numa_nodes" ] && exit
printf "machine has %s numa nodes\n" "$numa_nodes"

{
printf '{' ;
printf '"hostname":"%s",' "$(hostname)" ;
printf '"file_size":%s,' "$(du -b "$testfile" | awk '{print $1}')" ;
printf '"no_nodes":%s,' "$numa_nodes" ;
printf '"measurements":[' ;
} >> "$outfile"

for thread in $(seq 0 $((numa_nodes - 1))) ; do
	for pagecache in $(seq 0 $((numa_nodes - 1))) ; do
		for buffer in $(seq 0 $((numa_nodes - 1))) ; do
			./numabench -i "$iterations" --thread "$thread" --buffer "$buffer" --pagecache \
				"$pagecache" -m thread -m pages >> "$outfile"
			printf "," >> "$outfile"
		done
	done
done
sed -i '$ s/.$//' "$outfile"
printf "]}" >> "$outfile"

printf "saved to %s\n" "$outfile"
