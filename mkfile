MKSHELL=/bin/ksh
B=build

all:V: $B/monstersA-despeck.pbm $B/monstersA-regions.txt


#$B/%-unblue.png: %.jpg unblue
#	unblue 200 5 $stem.jpg $target

$B/%-thresh.pbm:D: $B/%-blur5.jpg threshold
	./threshold 0.79 $stem-blur5.jpg > $target

$B/%-despeck.pbm:D: $B/%-thresh.pbm despeckle
	set -o pipefail
	./despeckle 20000 $B/$stem-thresh.pbm |
        unblackedges > $target


$B/%-blur5.jpg: %.jpg
	convert $stem.jpg -gaussian-blur 0x5 $target

$B/%-blur7.jpg: %.jpg
	convert $stem.jpg -gaussian-blur 0x7 $target

find-regions: find-regions.c
	gcc -I/usr/include/netpbm -Wall -O2 -o $target $prereq -lnetpbm

$B/regions-%.txt: $B/%-despeck.pbm find-regions
	./find-regions $B/$stem-despeck.pbm $target

$B/%-coords.txt: $B/%-despeck.pbm find-regions
	./find-regions $stem-despeck.pbm $target

