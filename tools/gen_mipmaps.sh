#!/usr/bin/env bash
# yes, it relies on bash, sorry posix peoples

for thing in {p,n}*.{jpg,png}; do
	i=1;
	res=$(identify "$thing" | cut -f3 -d' ' | cut -f1 -d'x')

	while [[ $((res >> i)) != 0 ]]; do
		outres=$((res >> i))
		echo "$thing ($res x $res) -> mip$i-$thing ($outres x $outres)";
		convert -resize "$outres"x"$outres" -gaussian-blur 16x16 "$thing" "mip$i-$thing";
		(( i++ ));
	done;
done
