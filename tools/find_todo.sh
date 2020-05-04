#!/bin/sh
# script to find and print "TODO:" comments in files
# in a neat and pretty format
# originally from helix-os

if [ -z "$1" ]; then
	SRCDIRS="src include shaders"
else
	SRCDIRS="$1"
fi

for thing in $SRCDIRS; do
	# TODO: more C-like extensions (js, rust, ???)
	find $thing -type f \( -iname '*.cpp' -o -iname '*.hpp' \
	                       -o -iname '*.c' -o -iname '*.h' \) |
	while read fname; do 
		fgrep -l "TODO:" "$fname" | sed 's/.*/==> &/'
		fgrep -n "TODO:" "$fname" |
		sed 's/\([0-9]*\).*TODO:/    \1 -/g' |
		sed 's/\*\///g'
	done
done
